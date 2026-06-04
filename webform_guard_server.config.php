<?php

/**
 * @file
 * Configuration metadata for Webform Guard Server.
 */

/**
 * Implements hook_config_info().
 */
function webform_guard_server_config_info() {
  $config['webform_guard_server.settings'] = array(
    'label' => 'Webform Guard Server settings',
    'group' => 'system',
  );

  return $config;
}
