<?php
/**
 * @file
 *   Provide default views for nodehierarchy_views
 */

/**
 * Implements hook_views_default_views().
 */
function nodehierarchy_views_views_default_views() {
  $views = array();

  $view = new view();
  $view->name = 'nodehierarchy_children';
  $view->description = 'Display a node\'s children as teasers';
  $view->tag = '';
  $view->base_table = 'node';
  $view->human_name = 'Node Hierarchy Children';
  $view->core = 0;
  $view->api_version = '3.0';
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

  /* Display: Defaults */
  $handler = $view->new_display('default', 'Defaults', 'default');
  $handler->display->display_options['use_more_always'] = FALSE;
  $handler->display->display_options['access']['type'] = 'none';
  $handler->display->display_options['cache']['type'] = 'none';
  $handler->display->display_options['query']['type'] = 'views_query';
  $handler->display->display_options['exposed_form']['type'] = 'basic';
  $handler->display->display_options['pager']['type'] = 'full';
  $handler->display->display_options['style_plugin'] = 'default';
  $handler->display->display_options['row_plugin'] = 'node';
  /* Sort criterion: Node Hierarchy: Child Weight */
  $handler->display->display_options['sorts']['cweight']['id'] = 'cweight';
  $handler->display->display_options['sorts']['cweight']['table'] = 'nodehierarchy';
  $handler->display->display_options['sorts']['cweight']['field'] = 'cweight';
  /* Contextual filter: Node Hierarchy: Parent Node ID */
  $handler->display->display_options['arguments']['pnid']['id'] = 'pnid';
  $handler->display->display_options['arguments']['pnid']['table'] = 'nodehierarchy';
  $handler->display->display_options['arguments']['pnid']['field'] = 'pnid';
  $handler->display->display_options['arguments']['pnid']['default_action'] = 'not found';
  $handler->display->display_options['arguments']['pnid']['exception']['title_enable'] = TRUE;
  $handler->display->display_options['arguments']['pnid']['default_argument_type'] = 'fixed';
  $handler->display->display_options['arguments']['pnid']['summary']['format'] = 'default_summary';
  /* Filter criterion: Content: Published */
  $handler->display->display_options['filters']['status']['id'] = 'status';
  $handler->display->display_options['filters']['status']['table'] = 'node';
  $handler->display->display_options['filters']['status']['field'] = 'status';
  $handler->display->display_options['filters']['status']['value'] = 1;
  $handler->display->display_options['filters']['status']['group'] = 0;
  $handler->display->display_options['filters']['status']['expose']['operator'] = FALSE;

  /* Display: Children Teasers */
  $handler = $view->new_display('nodehierarchy_embed', 'Children Teasers', 'teasers');

  /* Display: Title List */
  $handler = $view->new_display('nodehierarchy_embed', 'Title List', 'titles');
  $handler->display->display_options['defaults']['style_plugin'] = FALSE;
  $handler->display->display_options['style_plugin'] = 'default';
  $handler->display->display_options['defaults']['style_options'] = FALSE;
  $handler->display->display_options['defaults']['row_plugin'] = FALSE;
  $handler->display->display_options['row_plugin'] = 'fields';
  $handler->display->display_options['defaults']['row_options'] = FALSE;
  $handler->display->display_options['defaults']['fields'] = FALSE;
  /* Field: Content: Title */
  $handler->display->display_options['fields']['title']['id'] = 'title';
  $handler->display->display_options['fields']['title']['table'] = 'node';
  $handler->display->display_options['fields']['title']['field'] = 'title';
  $handler->display->display_options['fields']['title']['label'] = '';
  $handler->display->display_options['fields']['title']['element_label_colon'] = FALSE;

  $views[$view->name] = $view;

  return $views;
}
