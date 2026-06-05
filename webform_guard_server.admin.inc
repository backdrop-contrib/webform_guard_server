<?php

/**
 * @file
 * Admin settings form for Webform Guard Server.
 */

/**
 * After-build callback: limit the subscription expiry year dropdown.
 *
 * Restricts the year select to the current year through current year + 10,
 * since expiry dates in the past are not useful when creating or editing clients.
 *
 * @param array $element
 *   The date form element after processing.
 *
 * @return array
 *   The modified element.
 */
function webform_guard_server_limit_expiry_years($element) {
  $current_year = (int) date('Y');
  $years = range($current_year, $current_year + 10);
  $element['year']['#options'] = array_combine($years, $years);
  return $element;
}

/**
 * Settings form callback.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 *
 * @return array
 *   The form render array.
 */
function webform_guard_server_settings_form($form, &$form_state) {
  $config = config('webform_guard_server.settings');

  $form['supplier_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Supplier name'),
    '#description' => t('The name of this service provider, shown to clients when they test their connection. E.g. "Albany Computers".'),
    '#default_value' => $config->get('supplier_name'),
    '#size' => 60,
    '#maxlength' => 255,
  );

  $domain_blocklist = $config->get('domain_blocklist') ?: array();
  $form['domain_blocklist'] = array(
    '#type' => 'textarea',
    '#title' => t('Domain blocklist'),
    '#description' => t('One domain pattern per line. Supports exact domains (<code>mail.ru</code>), suffix matching (<code>.ru</code>), and wildcards (<code>spam*.com</code>). Case-insensitive.'),
    '#default_value' => implode("\n", $domain_blocklist),
    '#rows' => 10,
  );

  $form['enable_report_spam'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable spam reporting'),
    '#description' => t('Allow clients to report spam submissions through this server and add identifiers to the shared blocklist.'),
    '#default_value' => $config->get('enable_report_spam'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save settings'),
  );

  return $form;
}

/**
 * Form submit handler.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function webform_guard_server_settings_form_submit($form, &$form_state) {
  $config = config('webform_guard_server.settings');
  $config->set('supplier_name', trim($form_state['values']['supplier_name']));
  $config->set('enable_report_spam', !empty($form_state['values']['enable_report_spam']));
  $domain_blocklist = array_map('strtolower', array_filter(
    array_map('trim', explode("\n", $form_state['values']['domain_blocklist']))
  ));
  $domain_blocklist = array_values(array_unique($domain_blocklist));
  sort($domain_blocklist);
  $config->set('domain_blocklist', $domain_blocklist);
  $config->save();

  backdrop_set_message(t('Webform Guard Server settings saved.'));
}

/**
 * Generate a strong random API key.
 *
 * Used when creating or editing client records.
 *
 * @return string
 *   A 40-character random API key.
 */
function webform_guard_server_generate_api_secret() {
  $characters = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()-_+=<>?';
  $secret = '';
  for ($i = 0; $i < 40; $i++) {
    $secret .= $characters[random_int(0, strlen($characters) - 1)];
  }
  return $secret;
}

/**
 * Page callback: list all spam identifiers in the blocklist.
 *
 * @return string
 *   Rendered HTML table with pager, or a paragraph if the blocklist is empty.
 */
function webform_guard_server_blocklist_page() {
  $header = array(
    array('data' => t('Identifier'), 'field' => 's.identifier', 'sort' => 'asc'),
    array('data' => t('Field'), 'field' => 's.field'),
    array('data' => t('Reports'), 'field' => 's.count'),
    array('data' => t('Last seen'), 'field' => 's.last_seen'),
    t('Reported by'),
    array('data' => t('First added'), 'field' => 's.created'),
    t('Actions'),
  );

  $query = db_select('webform_guard_spam_identifier', 's')
    ->fields('s')
    ->extend('TableSort')
    ->orderByHeader($header)
    ->extend('PagerDefault')
    ->limit(50);
  $query->leftJoin('webform_guard_clients', 'c', 's.site_identifier = c.site_identifier');
  $query->addField('c', 'label', 'client_label');

  $result = $query->execute();
  $rows = array();

  foreach ($result as $row) {
    $reported_by = !empty($row->client_label)
      ? check_plain($row->client_label)
      : (!empty($row->site_identifier) ? check_plain($row->site_identifier) : t('—'));

    $rows[] = array(
      check_plain($row->identifier),
      check_plain($row->field),
      (int) $row->count,
      $row->last_seen ? format_date($row->last_seen, 'short') : t('—'),
      $reported_by,
      format_date($row->created, 'short'),
      l(t('Remove'), 'admin/config/webform-guard-server/blocklist/' . $row->id . '/delete'),
    );
  }

  if (empty($rows)) {
    return '<p>' . t('The blocklist is empty.') . '</p>';
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= theme('pager');
  return $output;
}

/**
 * Confirm form for removing an identifier from the blocklist.
 *
 * @param int $id
 *   The spam identifier record ID.
 */
function webform_guard_server_blocklist_delete_form($form, &$form_state, $id) {
  $record = db_select('webform_guard_spam_identifier', 's')
    ->fields('s')
    ->condition('id', $id)
    ->execute()
    ->fetchObject();

  if (!$record) {
    backdrop_set_message(t('Record not found.'), 'error');
    backdrop_goto('admin/config/webform-guard-server/blocklist');
  }

  $form['#record_id'] = $id;

  return confirm_form(
    $form,
    t('Remove %identifier from the blocklist?', array('%identifier' => $record->identifier)),
    'admin/config/webform-guard-server/blocklist',
    t('This will allow future submissions from this address to pass through the spam check.'),
    t('Remove'),
    t('Cancel')
  );
}

/**
 * Submit handler for the blocklist delete confirm form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function webform_guard_server_blocklist_delete_form_submit($form, &$form_state) {
  db_delete('webform_guard_spam_identifier')
    ->condition('id', $form['#record_id'])
    ->execute();

  backdrop_set_message(t('Identifier removed from blocklist.'));
  $form_state['redirect'] = 'admin/config/webform-guard-server/blocklist';
}

/**
 * Form callback: manually add an identifier to the blocklist.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 *
 * @return array
 *   The form render array.
 */
function webform_guard_server_blocklist_add_form($form, &$form_state) {
  $form['identifier_type'] = array(
    '#type' => 'select',
    '#title' => t('Field type'),
    '#description' => t('The type of identifier being added. Used to normalise the value before storing.'),
    '#options' => array(
      'email' => t('Email address'),
      'tel' => t('Telephone number'),
      'url' => t('Website URL'),
      'textfield' => t('Text (name, keyword, phrase)'),
    ),
    '#required' => TRUE,
    '#default_value' => 'email',
  );

  $form['identifier_value'] = array(
    '#type' => 'textfield',
    '#title' => t('Identifier value'),
    '#description' => t('The value to block, e.g. an email address or keyword. It will be normalised before storing.'),
    '#required' => TRUE,
    '#size' => 80,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add to blocklist'),
  );

  return $form;
}

/**
 * Submit handler for the manual blocklist add form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function webform_guard_server_blocklist_add_form_submit($form, &$form_state) {
  $type = $form_state['values']['identifier_type'];
  $value = trim($form_state['values']['identifier_value']);

  $added = webform_guard_server_add_spam_identifier('manual', $value, $type);

  if ($added) {
    backdrop_set_message(t('Identifier added to blocklist.'));
  }
  else {
    backdrop_set_message(t('The identifier could not be added. It may be empty after normalisation.'), 'warning');
  }

  $form_state['redirect'] = 'admin/config/webform-guard-server/blocklist';
}

/**
 * Page callback: list all subscription tiers.
 *
 * @return string
 *   Rendered HTML table of tiers, or a prompt to add one if none exist.
 */
function webform_guard_server_tiers_page() {
  $header = array(
    t('Name'),
    t('Limit'),
    t('Period'),
    t('Trial days'),
    t('Actions'),
  );

  $result = db_select('webform_guard_tiers', 't')
    ->fields('t')
    ->orderBy('weight')
    ->orderBy('name')
    ->execute();

  $rows = array();
  foreach ($result as $tier) {
    $limit = $tier->check_limit !== NULL
      ? number_format((int) $tier->check_limit)
      : t('Unlimited');
    $period = $tier->check_period ? check_plain($tier->check_period) : t('—');
    $trial = $tier->trial_days
      ? (int) $tier->trial_days . ' ' . t('days')
      : t('—');

    $actions = array(
      l(t('Edit'), 'admin/config/webform-guard-server/tiers/' . $tier->id . '/edit'),
      l(t('Delete'), 'admin/config/webform-guard-server/tiers/' . $tier->id . '/delete'),
    );

    $rows[] = array(
      check_plain($tier->name),
      $limit,
      $period,
      $trial,
      implode(' | ', $actions),
    );
  }

  if (empty($rows)) {
    return '<p>' . t('No tiers defined. <a href="@url">Add a tier</a>.', array(
      '@url' => url('admin/config/webform-guard-server/tiers/add'),
    )) . '</p>';
  }

  return theme('table', array('header' => $header, 'rows' => $rows));
}

/**
 * Form callback: add or edit a subscription tier.
 *
 * @param int|null $tier_id
 *   The tier ID to edit, or NULL when adding a new tier.
 */
function webform_guard_server_tier_form($form, &$form_state, $tier_id = NULL) {
  $tier = NULL;
  if (!empty($tier_id)) {
    $tier = db_select('webform_guard_tiers', 't')
      ->fields('t')
      ->condition('id', $tier_id)
      ->execute()
      ->fetchObject();

    if (!$tier) {
      backdrop_set_message(t('Tier not found.'), 'error');
      backdrop_goto('admin/config/webform-guard-server/tiers');
    }
  }

  $form['#tier_id'] = $tier_id;

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Human-readable name for this tier, e.g. "Free Trial" or "Tier 1".'),
    '#default_value' => $tier ? $tier->name : '',
    '#required' => TRUE,
  );

  $form['check_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Check limit'),
    '#description' => t('Maximum submission checks per period. Leave blank for unlimited.'),
    '#default_value' => ($tier && $tier->check_limit !== NULL) ? $tier->check_limit : '',
    '#size' => 10,
  );

  $form['check_period'] = array(
    '#type' => 'select',
    '#title' => t('Check period'),
    '#description' => t('How often the check limit resets.'),
    '#options' => array(
      '' => t('Unlimited / no reset'),
      'daily' => t('Daily'),
      'monthly' => t('Monthly'),
      'annual' => t('Annual'),
      'total' => t('Total (lifetime)'),
    ),
    '#default_value' => ($tier && $tier->check_period) ? $tier->check_period : '',
  );

  $form['trial_days'] = array(
    '#type' => 'textfield',
    '#title' => t('Trial days'),
    '#description' => t('If set, the client expiry date is auto-populated to this many days from now when this tier is assigned. Leave blank for non-trial tiers.'),
    '#default_value' => ($tier && $tier->trial_days) ? $tier->trial_days : '',
    '#size' => 5,
  );

  $form['weight'] = array(
    '#type' => 'textfield',
    '#title' => t('Weight'),
    '#description' => t('Display order. Lower numbers appear first.'),
    '#default_value' => $tier ? $tier->weight : 0,
    '#size' => 5,
  );

  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#description' => t('Optional admin notes about this tier.'),
    '#default_value' => ($tier && $tier->description) ? $tier->description : '',
    '#rows' => 3,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => $tier_id ? t('Save tier') : t('Add tier'),
  );

  return $form;
}

/**
 * Validate handler for the tier add/edit form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function webform_guard_server_tier_form_validate($form, &$form_state) {
  $check_limit = trim($form_state['values']['check_limit']);
  if ($check_limit !== '' && (!is_numeric($check_limit) || (int) $check_limit < 1)) {
    form_set_error('check_limit', t('Check limit must be a positive integer, or leave blank for unlimited.'));
  }

  $trial_days = trim($form_state['values']['trial_days']);
  if ($trial_days !== '' && (!is_numeric($trial_days) || (int) $trial_days < 1)) {
    form_set_error('trial_days', t('Trial days must be a positive integer, or leave blank.'));
  }

  if (!is_numeric(trim($form_state['values']['weight']))) {
    form_set_error('weight', t('Weight must be a number.'));
  }
}

/**
 * Submit handler for the tier add/edit form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function webform_guard_server_tier_form_submit($form, &$form_state) {
  $tier_id = $form['#tier_id'];
  $check_limit = trim($form_state['values']['check_limit']);
  $trial_days = trim($form_state['values']['trial_days']);

  $fields = array(
    'name' => trim($form_state['values']['name']),
    'check_limit' => $check_limit !== '' ? (int) $check_limit : NULL,
    'check_period' => $form_state['values']['check_period'] ?: NULL,
    'trial_days' => $trial_days !== '' ? (int) $trial_days : NULL,
    'weight' => (int) $form_state['values']['weight'],
    'description' => trim($form_state['values']['description']),
  );

  if ($tier_id) {
    db_update('webform_guard_tiers')
      ->fields($fields)
      ->condition('id', $tier_id)
      ->execute();
    backdrop_set_message(t('Tier updated.'));
  }
  else {
    $fields['created'] = REQUEST_TIME;
    db_insert('webform_guard_tiers')
      ->fields($fields)
      ->execute();
    backdrop_set_message(t('Tier added.'));
  }

  $form_state['redirect'] = 'admin/config/webform-guard-server/tiers';
}

/**
 * Confirm form for deleting a tier.
 *
 * Prevents deletion if any clients are currently assigned to the tier.
 *
 * @param int $tier_id
 *   The ID of the tier to delete.
 */
function webform_guard_server_tier_delete_form($form, &$form_state, $tier_id) {
  $tier = db_select('webform_guard_tiers', 't')
    ->fields('t')
    ->condition('id', $tier_id)
    ->execute()
    ->fetchObject();

  if (!$tier) {
    backdrop_set_message(t('Tier not found.'), 'error');
    backdrop_goto('admin/config/webform-guard-server/tiers');
  }

  $client_count = db_select('webform_guard_clients', 'c')
    ->condition('tier_id', $tier_id)
    ->countQuery()
    ->execute()
    ->fetchField();

  if ($client_count > 0) {
    backdrop_set_message(
      t('The tier %name cannot be deleted because @count client(s) are assigned to it. Reassign those clients first.', array(
        '%name' => $tier->name,
        '@count' => $client_count,
      )),
      'error'
    );
    backdrop_goto('admin/config/webform-guard-server/tiers');
  }

  $form['#tier_id'] = $tier_id;

  return confirm_form(
    $form,
    t('Delete the tier %name?', array('%name' => $tier->name)),
    'admin/config/webform-guard-server/tiers',
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Submit handler for the tier delete confirm form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function webform_guard_server_tier_delete_form_submit($form, &$form_state) {
  db_delete('webform_guard_tiers')
    ->condition('id', $form['#tier_id'])
    ->execute();

  backdrop_set_message(t('Tier deleted.'));
  $form_state['redirect'] = 'admin/config/webform-guard-server/tiers';
}

/**
 * Page callback: list all registered client sites.
 *
 * @return string
 *   Rendered HTML table of clients, or a prompt to add one if none exist.
 */
function webform_guard_server_clients_page() {
  $header = array(
    t('Label'),
    t('Email'),
    t('Tier'),
    t('Site identifier'),
    t('Subscription'),
    t('Expires'),
    t('Checks'),
    t('Blocked'),
    t('Auto-renew'),
    t('Last seen'),
    t('Created'),
    t('Actions'),
  );

  $rows = array();
  $query = db_select('webform_guard_clients', 'c')
    ->fields('c')
    ->orderBy('c.label');
  $query->leftJoin('webform_guard_site_metrics', 'm', 'c.site_identifier = m.site_identifier');
  $query->addField('m', 'blocked_count', 'blocked_count');
  $query->leftJoin('webform_guard_tiers', 't', 'c.tier_id = t.id');
  $query->addField('t', 'name', 'tier_name');
  $result = $query->execute();

  foreach ($result as $client) {
    $expires = !empty($client->subscription_expires)
      ? format_date($client->subscription_expires, 'short')
      : t('—');

    $actions = array(
      l(t('Edit'), 'admin/config/webform-guard-server/clients/' . $client->id . '/edit'),
      l(t('Delete'), 'admin/config/webform-guard-server/clients/' . $client->id . '/delete'),
    );

    $last_seen = !empty($client->last_seen)
      ? format_date($client->last_seen, 'short')
      : t('Never');

    $rows[] = array(
      check_plain($client->label),
      check_plain($client->email),
      $client->tier_name ? check_plain($client->tier_name) : t('—'),
      check_plain($client->site_identifier),
      check_plain($client->subscription_status),
      $expires,
      number_format((int) $client->check_count),
      number_format((int) $client->blocked_count),
      $client->auto_renew ? t('Yes') : t('No'),
      $last_seen,
      format_date($client->created, 'short'),
      implode(' | ', $actions),
    );
  }

  if (empty($rows)) {
    $output = '<p>' . t('No clients registered yet. <a href="@url">Add a client</a>.', array(
      '@url' => url('admin/config/webform-guard-server/clients/add'),
    )) . '</p>';
    return $output;
  }

  return theme('table', array('header' => $header, 'rows' => $rows));
}

/**
 * Form callback: add or edit a client site.
 *
 * @param int|null $client_id
 *   The client ID to edit, or NULL when adding a new client.
 */
function webform_guard_server_client_form($form, &$form_state, $client_id = NULL) {
  $client = NULL;
  if (!empty($client_id)) {
    $client = db_select('webform_guard_clients', 'c')
      ->fields('c')
      ->condition('id', $client_id)
      ->execute()
      ->fetchObject();

    if (!$client) {
      backdrop_set_message(t('Client not found.'), 'error');
      backdrop_goto('admin/config/webform-guard-server/clients');
    }
  }

  $form['#client_id'] = $client_id;

  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#description' => t('A human-readable name for this client, e.g. "Acme Ltd website".'),
    '#default_value' => $client ? $client->label : '',
    '#required' => TRUE,
  );

  $form['email'] = array(
    '#type' => 'textfield',
    '#title' => t('Contact email'),
    '#description' => t('Email address for this client, used for monthly spam reports and notifications.'),
    '#default_value' => $client ? $client->email : '',
    '#size' => 60,
    '#maxlength' => 255,
  );

  $tiers = db_select('webform_guard_tiers', 't')
    ->fields('t')
    ->orderBy('weight')
    ->orderBy('name')
    ->execute()
    ->fetchAllAssoc('id');

  $tier_options = array('' => t('— None —'));
  $tier_js_data = array();
  foreach ($tiers as $tier) {
    $tier_options[$tier->id] = check_plain($tier->name);
    $tier_js_data[$tier->id] = array(
      'trial_days' => $tier->trial_days ? (int) $tier->trial_days : NULL,
    );
  }

  $form['tier_id'] = array(
    '#type' => 'select',
    '#title' => t('Subscription tier'),
    '#description' => t('The tier determines check limits and trial conditions. Selecting a trial tier will auto-populate the expiry date.'),
    '#options' => $tier_options,
    '#default_value' => ($client && $client->tier_id) ? $client->tier_id : '',
    '#attributes' => array('id' => 'edit-tier-id'),
  );

  $form['auto_renew'] = array(
    '#type' => 'checkbox',
    '#title' => t('Auto-renew reminder'),
    '#description' => t('Flag this client for manual renewal when their subscription expires.'),
    '#default_value' => $client ? (int) $client->auto_renew : 0,
  );

  backdrop_add_js(
    array('webformGuardServer' => array('tiers' => $tier_js_data)),
    'setting'
  );
  backdrop_add_js(
    backdrop_get_path('module', 'webform_guard_server') . '/js/webform_guard_server.admin.js'
  );

  $form['site_identifier'] = array(
    '#type' => 'textfield',
    '#title' => t('Site identifier'),
    '#description' => t('A unique identifier for this client site. Recommended: the site domain, e.g. acme.co.uk. Must match what the client module sends.'),
    '#default_value' => $client ? $client->site_identifier : '',
    '#required' => TRUE,
  );

  $default_api_key = $client ? $client->api_key : '';
  if (isset($form_state['values']['api_key'])) {
    $default_api_key = $form_state['values']['api_key'];
  }

  $form['api_key'] = array(
    '#type' => 'textfield',
    '#title' => t('API key'),
    '#description' => t('The API key this client uses to authenticate. Must be at least 32 characters.'),
    '#default_value' => $default_api_key,
    '#required' => TRUE,
  );

  $form['generate_api_key'] = array(
    '#type' => 'submit',
    '#value' => t('Generate API key'),
    '#submit' => array('webform_guard_server_client_generate_api_key_submit'),
    '#limit_validation_errors' => array(),
    '#weight' => 1,
  );

  $form['subscription_status'] = array(
    '#type' => 'select',
    '#title' => t('Subscription status'),
    '#options' => array(
      'free' => t('Free'),
      'active' => t('Active'),
      'expired' => t('Expired'),
    ),
    '#default_value' => $client ? $client->subscription_status : 'free',
    '#required' => TRUE,
  );

  $form['subscription_expires'] = array(
    '#type' => 'date',
    '#title' => t('Subscription expiry date'),
    '#description' => t('Leave blank for free or non-expiring accounts.'),
    '#after_build' => array('webform_guard_server_limit_expiry_years'),
    '#default_value' => ($client && !empty($client->subscription_expires))
      ? array(
          'year' => date('Y', $client->subscription_expires),
          'month' => date('n', $client->subscription_expires),
          'day' => date('j', $client->subscription_expires),
        )
      : NULL,
  );

  $form['notes'] = array(
    '#type' => 'textarea',
    '#title' => t('Notes'),
    '#description' => t('Optional internal notes about this client.'),
    '#default_value' => $client ? $client->notes : '',
    '#rows' => 3,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => $client_id ? t('Save client') : t('Add client'),
    '#weight' => 10,
  );

  return $form;
}

/**
 * Submit handler: generate a random API key for a client.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array, updated in place with the generated key.
 */
function webform_guard_server_client_generate_api_key_submit($form, &$form_state) {
  $form_state['values']['api_key'] = webform_guard_server_generate_api_secret();
  $form_state['input']['api_key'] = $form_state['values']['api_key'];
  $form_state['rebuild'] = TRUE;
}

/**
 * Validate handler for the client add/edit form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function webform_guard_server_client_form_validate($form, &$form_state) {
  $api_key = trim($form_state['values']['api_key']);
  if (strlen($api_key) < 32) {
    form_set_error('api_key', t('The API key must be at least 32 characters long.'));
  }

  $site_identifier = trim($form_state['values']['site_identifier']);
  $client_id = $form['#client_id'];

  $query = db_select('webform_guard_clients', 'c')
    ->fields('c', array('id'))
    ->condition('site_identifier', $site_identifier);
  if (!empty($client_id)) {
    $query->condition('id', $client_id, '<>');
  }
  if ($query->execute()->fetchField()) {
    form_set_error('site_identifier', t('A client with this site identifier already exists.'));
  }

  $query = db_select('webform_guard_clients', 'c')
    ->fields('c', array('id'))
    ->condition('api_key', $api_key);
  if (!empty($client_id)) {
    $query->condition('id', $client_id, '<>');
  }
  if ($query->execute()->fetchField()) {
    form_set_error('api_key', t('This API key is already in use by another client.'));
  }
}

/**
 * Submit handler for the client add/edit form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function webform_guard_server_client_form_submit($form, &$form_state) {
  $client_id = $form['#client_id'];

  $expires = NULL;
  $date = $form_state['values']['subscription_expires'];
  if (!empty($date['year']) && !empty($date['month']) && !empty($date['day'])) {
    $expires = mktime(23, 59, 59, $date['month'], $date['day'], $date['year']);
  }

  $tier_id = $form_state['values']['tier_id'] ?: NULL;
  $fields = array(
    'label' => trim($form_state['values']['label']),
    'email' => trim($form_state['values']['email']),
    'tier_id' => $tier_id,
    'auto_renew' => !empty($form_state['values']['auto_renew']) ? 1 : 0,
    'site_identifier' => trim($form_state['values']['site_identifier']),
    'api_key' => trim($form_state['values']['api_key']),
    'subscription_status' => $form_state['values']['subscription_status'],
    'subscription_expires' => $expires,
    'notes' => trim($form_state['values']['notes']),
  );

  if ($client_id) {
    db_update('webform_guard_clients')
      ->fields($fields)
      ->condition('id', $client_id)
      ->execute();
    backdrop_set_message(t('Client updated.'));
  }
  else {
    $fields['created'] = REQUEST_TIME;
    db_insert('webform_guard_clients')
      ->fields($fields)
      ->execute();
    backdrop_set_message(t('Client added.'));
  }

  $form_state['redirect'] = 'admin/config/webform-guard-server/clients';
}

/**
 * Confirm form for deleting a client.
 *
 * @param int $client_id
 *   The ID of the client to delete.
 */
function webform_guard_server_client_delete_form($form, &$form_state, $client_id) {
  $client = db_select('webform_guard_clients', 'c')
    ->fields('c')
    ->condition('id', $client_id)
    ->execute()
    ->fetchObject();

  if (!$client) {
    backdrop_set_message(t('Client not found.'), 'error');
    backdrop_goto('admin/config/webform-guard-server/clients');
  }

  $form['#client_id'] = $client_id;

  return confirm_form(
    $form,
    t('Are you sure you want to delete the client %label?', array('%label' => $client->label)),
    'admin/config/webform-guard-server/clients',
    t('This action cannot be undone. The client will no longer be able to authenticate with this server.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Submit handler for the client delete confirm form.
 *
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function webform_guard_server_client_delete_form_submit($form, &$form_state) {
  db_delete('webform_guard_clients')
    ->condition('id', $form['#client_id'])
    ->execute();

  backdrop_set_message(t('Client deleted.'));
  $form_state['redirect'] = 'admin/config/webform-guard-server/clients';
}
