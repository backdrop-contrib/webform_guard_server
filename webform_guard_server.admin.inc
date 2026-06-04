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
  $config->set('enable_report_spam', !empty($form_state['values']['enable_report_spam']));
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
 * Page callback: list all registered client sites.
 *
 * @return string
 *   Rendered HTML table of clients, or a prompt to add one if none exist.
 */
function webform_guard_server_clients_page() {
  $header = array(
    t('Label'),
    t('Site identifier'),
    t('Subscription'),
    t('Expires'),
    t('Checks'),
    t('Last seen'),
    t('Created'),
    t('Actions'),
  );

  $rows = array();
  $result = db_select('webform_guard_clients', 'c')
    ->fields('c')
    ->orderBy('label')
    ->execute();

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
      check_plain($client->site_identifier),
      check_plain($client->subscription_status),
      $expires,
      number_format((int) $client->check_count),
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

  $fields = array(
    'label' => trim($form_state['values']['label']),
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
