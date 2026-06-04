<?php

/**
 * @file
 * Views integration for Webform Guard Server.
 */

/**
 * Implements hook_views_data().
 */
function webform_guard_server_views_data() {
  $data = array();
  $data['webform_guard_spam_identifier']['table']['group'] = t('Webform Guard');
  $data['webform_guard_spam_identifier']['table']['base'] = array(
    'field' => 'id',
    'title' => t('Spam identifier'),
    'help' => t('Spam identifiers reported by Webform Guard clients.'),
  );

  $data['webform_guard_spam_identifier']['field'] = array(
    'title' => t('Identifier field'),
    'help' => t('The form field key used to identify repeat spam submissions.'),
    'field' => array('handler' => 'views_handler_field_default'),
    'filter' => array('handler' => 'views_handler_filter_in_operator'),
    'sort' => array('handler' => 'views_handler_sort'),
  );

  $data['webform_guard_spam_identifier']['identifier'] = array(
    'title' => t('Identifier value'),
    'help' => t('The normalized value used to identify repeat spam submissions.'),
    'field' => array('handler' => 'views_handler_field_default'),
    'filter' => array('handler' => 'views_handler_filter_string'),
    'sort' => array('handler' => 'views_handler_sort'),
  );

  $data['webform_guard_spam_identifier']['count'] = array(
    'title' => t('Spam count'),
    'help' => t('The number of times this identifier was reported as spam.'),
    'field' => array('handler' => 'views_handler_field_numeric'),
    'filter' => array('handler' => 'views_handler_filter_numeric'),
    'sort' => array('handler' => 'views_handler_sort'),
  );

  $data['webform_guard_spam_identifier']['last_seen'] = array(
    'title' => t('Last seen'),
    'help' => t('The most recent time this identifier was reported as spam.'),
    'field' => array('handler' => 'views_handler_field_date'),
    'filter' => array('handler' => 'views_handler_filter_date'),
    'sort' => array('handler' => 'views_handler_sort_date'),
  );

  $data['webform_guard_spam_identifier']['site_identifier'] = array(
    'title' => t('Site identifier'),
    'help' => t('The site identifier reported by the client when the spam identifier was seen.'),
    'field' => array('handler' => 'views_handler_field_default'),
    'filter' => array('handler' => 'views_handler_filter_string'),
    'sort' => array('handler' => 'views_handler_sort'),
  );

  $data['webform_guard_spam_identifier']['created'] = array(
    'title' => t('First reported'),
    'help' => t('The date this identifier was first reported as spam.'),
    'field' => array('handler' => 'views_handler_field_date'),
    'filter' => array('handler' => 'views_handler_filter_date'),
    'sort' => array('handler' => 'views_handler_sort_date'),
  );

  $data['webform_guard_site_metrics']['table']['group'] = t('Webform Guard');
  $data['webform_guard_site_metrics']['table']['base'] = array(
    'field' => 'id',
    'title' => t('Site metrics'),
    'help' => t('Site-level blocked submission counts reported by Webform Guard.'),
  );

  $data['webform_guard_site_metrics']['site_identifier'] = array(
    'title' => t('Site identifier'),
    'help' => t('The unique identifier for the client site that reported blocked submissions.'),
    'field' => array('handler' => 'views_handler_field_default'),
    'filter' => array('handler' => 'views_handler_filter_string'),
    'sort' => array('handler' => 'views_handler_sort'),
  );

  $data['webform_guard_site_metrics']['blocked_count'] = array(
    'title' => t('Blocked count'),
    'help' => t('The number of blocked submissions recorded for this site.'),
    'field' => array('handler' => 'views_handler_field_numeric'),
    'filter' => array('handler' => 'views_handler_filter_numeric'),
    'sort' => array('handler' => 'views_handler_sort'),
  );

  $data['webform_guard_site_metrics']['last_blocked'] = array(
    'title' => t('Last blocked'),
    'help' => t('The timestamp of the most recent blocked submission for this site.'),
    'field' => array('handler' => 'views_handler_field_date'),
    'filter' => array('handler' => 'views_handler_filter_date'),
    'sort' => array('handler' => 'views_handler_sort_date'),
  );

  $data['webform_guard_site_metrics']['created'] = array(
    'title' => t('Created'),
    'help' => t('The time this site metrics record was first created.'),
    'field' => array('handler' => 'views_handler_field_date'),
    'filter' => array('handler' => 'views_handler_filter_date'),
    'sort' => array('handler' => 'views_handler_sort_date'),
  );

  return $data;
}
