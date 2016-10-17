<?php

/**
 * @file
 * Contains the children embed display plugin.
 */

/**
 * A plugin to handle defaults on a view.
 *
 * @ingroup views_display_plugins
 */

class nodehierarchy_views_plugin_display_embed extends views_plugin_display {

/**
   * Provide the summary for page options in the views UI.
   *
   * This output is returned as an array.
   */
  function options_summary(&$categories, &$options) {
    // It is very important to call the parent function here:
    parent::options_summary($categories, $options);

    $categories['nodehierarchy_embed'] = array(
      'title' => t('Embed settings'),
      'column' => 'second',
      'build' => array(
        '#weight' => -10,
      ),
    );

    $name = strip_tags($this->get_option('nodehierarchy_embed_admin_name'));
    if (empty($name)) {
      $name = t('None');
    }

    $options['nodehierarchy_embed_admin_name'] = array(
      'category' => 'nodehierarchy_embed',
      'title' => t('Admin name'),
      'value' => views_ui_truncate($name, 24),
    );
  }

  /**
   * Provide the default form for setting options.
   */
  function options_form(&$form, &$form_state) {
    // It is very important to call the parent function here:
    parent::options_form($form, $form_state);

    switch ($form_state['section']) {
      case 'nodehierarchy_embed_admin_name':
        $form['#title'] .= t('Embed admin name');
        $form['nodehierarchy_embed_admin_name'] = array(
          '#type' => 'textfield',
          '#description' => t('This will appear as the name of this embed in the node edit screen.'),
          '#default_value' => $this->get_option('nodehierarchy_embed_admin_name'),
        );
        break;
    }
  }

  /**
   * Perform any necessary changes to the form values prior to storage.
   * There is no need for this function to actually store the data.
   */
  function options_submit(&$form, &$form_state) {
    // It is very important to call the parent function here:
    parent::options_submit($form, $form_state);
    switch ($form_state['section']) {
      case 'nodehierarchy_embed_admin_name':
        $this->set_option('nodehierarchy_embed_admin_name', $form_state['values']['nodehierarchy_embed_admin_name']);
        break;
    }
  }

  /**
   * The display block handler returns the structure necessary for a block.
   */
  function execute() {
    // Prior to this being called, the $view should already be set to this
    // display, and arguments should be set on the view.
    if (!isset($this->view->override_path)) {
      $this->view->override_path = 'node';
    }

    $data = $this->view->render();
    if (!empty($this->view->result) || $this->get_option('empty') || !empty($this->view->style_plugin->definition['even empty'])) {
      return $data;
    }
  }

}
