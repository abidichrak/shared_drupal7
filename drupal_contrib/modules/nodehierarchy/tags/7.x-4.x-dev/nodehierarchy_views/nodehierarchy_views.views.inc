<?php

/**
 * Implements hook_views_plugins().
 */
function nodehierarchy_views_views_plugins() {
  return array(
    'display' => array(
      'nodehierarchy_embed' => array(
        'title' => t('Node Hierarachy Embed'),
        'help' => t('A display for embedding Node Hierarchy children on a node.'),
        'handler' => 'nodehierarchy_views_plugin_display_embed',
        'theme' => 'views_view',
        'theme path' => drupal_get_path('module', 'views') . '/theme',
        'nodehierarchy embed view' => TRUE,
        'use ajax' => TRUE,
        'use pager' => TRUE,
        'use more' => TRUE,
        'accept attachments' => TRUE,
        'help topic' => 'nodehierarchy_embed',
      ),
    ),
  );
}
