<?php

/**
 * @file
 * Implements necessary callbacks for Calendar Systems administration forms.
 */

/**
 * Form callback to list all available calendars and their config links.
 *
 * @return
 *   An array of form elements to be themed later on.
 *
 * @ingroup forms
 */
function calendar_systems_calendars_form() {
  $form = array();

  // Include helpers.
  module_load_include('inc', 'calendar_systems');
  // Do not show the calendars form in case
  // that the required core patch is not yet applied.
  // TODO: Implement a requirements checker helper, etc.
  if (!_calendar_systems_patches_applied()) {
    $form['calendar_systems_error'] = array(
      '#type'  => 'item',
      '#value' => t('Calendar Systems is not correctly installed. Checkout the <a href="!link">status reports</a>.', array(
        '!link' => url('admin/reports/status')
      )),
    );
    // Let it be rendered.
    return $form;
  }

  $options = array();
  $calendars = calendar_systems_calendars();

  // Generate site-wide options.
  $form['sitewide'] = array();
  foreach ($calendars as $identifier => $calendar) {
    $options[$identifier] = '';

    // Add fake elements to be themed later on.
    $form['sitewide'][$calendar['name']]['_id'] = array(
      '#value' => $identifier,
    );

    $form['sitewide'][$calendar['name']]['_config'] = array(
      '#value' => function_exists($calendar['config callback']) ?
        l(t('Configuration'), 'admin/settings/date-time/calendars/' . $identifier) :
        t('No configuration'),
    );
  }

  $form['default_calendar'] = array(
    '#type' => 'radios',
    '#default_value' => variable_get('calendar_systems_default_calendar', 'gregorian'),
    '#options' => $options,
  );

  // Note for language specific calendar configs.
  global $user;
  $form['calendar_systems_notice'] = array('#type' => 'item');
  if (function_exists('locale')) {
    $form['calendar_systems_notice']['#value'] = t('Calendars are also configurable per <a href="!locale-link">locale</a> and <a href="!user-link">user</a> basis.', array(
      '!user-link' => url("user/{$user->uid}/edit"),
      '!locale-link' => url('admin/settings/language'),
    ));
  }
  else {
    $form['calendar_systems_notice']['#value'] = t('<strong>Note</strong> that calendars are also configurable per language basis if you had <a href="!link">Locale</a> module enabled.',
      array('!link' => url('admin/build/modules'))
    );
  }

  // Append the submit button.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Set as default calendar'),
  );

  return $form;
}


/**
 * Form submission callback.
 *
 * @ingroup form_submit
 */
function calendar_systems_calendars_form_submit($form, &$form_state) {
  if ($form_state['values']['default_calendar']) {
    variable_set('calendar_systems_default_calendar', $form_state['values']['default_calendar']);
    drupal_set_message(t('System default calendar has been updated.'));
  }
}


/**
 * Theme callback for Calendar Systems admin form.
 *
 * @param $form
 *   Admin form array.
 *
 * @ingroup form_theme
 */
function theme_calendar_systems_calendars_form($form) {
  // Include helpers.
  module_load_include('inc', 'calendar_systems');
  // Register theme callbacks only if the patch is applied.
  if (_calendar_systems_patches_applied()) {
    $rows = array();
    // Set form table rows.
    foreach ($form['sitewide'] as $name => $element) {
      if (!isset($element['_id']) || !is_array($element['_id'])) {
        continue;
      }
      $rows[] = array(
        drupal_render($form['default_calendar'][$element['_id']['#value']]),
        check_plain($name),
        drupal_render($element['_config']),
      );
    }
    unset($form['sitewide']);

    // Sets form table header.
    $header = array(
      t('Default'),
      t('Calendar'),
      array(
        'colspan' => 1,
        'data' => t('Actions'),
      )
    );

    $output  = '<h3>' . t('System Default Calendar') . '</h3>';
    $output .= theme('table', $header, $rows);
    $output .= drupal_render($form);
    return $output;
  }

  // Patch is not yet applied.
  return drupal_render($form);
}


/**
 * Form callback for a calendar admin settings.
 *
 * @param $identifier
 *   Calendar identifier.
 *
 * @return
 *   An array of form elements.
 *
 * @ingroup forms
 */
function calendar_systems_calendar_form(&$form_state, $identifier) {
  $calendar = calendar_systems_calendars('calendar', $identifier);

  if ($calendar && function_exists($calendar['config callback'])) {
    $form = $calendar['config callback']($calendar['config']);

    // Needed for validation and submission callbacks.
    $form['calendar'] = array(
      '#type' => 'value',
      '#value' => $calendar,
    );

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
    );

    // Set calendar configuration page title.
    drupal_set_title(t('@calendar Configuration',
      array('@calendar' => calendar_systems_calendar_title($identifier))
    ));

    return $form;
  }

  drupal_not_found();
  exit();
}


/**
 * Form validation callback for a calendar admin settings.
 *
 * @ingroup form_validate
 */
function calendar_systems_calendar_form_validate($form, &$form_state) {
  // Pass to calendar form validator.
  $function = $form_state['values']['calendar']['config callback'] . '_validate';
  if (function_exists($function)) {
    $function($form, $form_state);
  }
}


/**
 * Form submission callback for a calendar admin settings.
 *
 * @ingroup form_submit
 */
function calendar_systems_calendar_form_submit($form, &$form_state) {
  $calendar = $form_state['values']['calendar'];

  // Saving necessary configs.
  unset($form_state['values']['op'],
    $form_state['values']['submit'],
    $form_state['values']['form_id'],
    $form_state['values']['calendar'],
    $form_state['values']['form_token'],
    $form_state['values']['form_build_id']
  );
  variable_set('calendar_systems_settings_' . $calendar['identifier'], $form_state['values']);

  $form_state['redirect'] = 'admin/settings/date-time/calendars';
  drupal_set_message(t('Configuration for <em>@calendar</em> has been successfully saved.',
    array('@calendar' => calendar_systems_calendar_title($calendar['identifier']))
  ));
}


/**
 * Theme callback for Locale language overview form.
 * It's also compatible with admin_language.module which
 * overrides the same themer callback.
 *
 * @param $form
 *   Language overview form array.
 *
 * @ingroup form_theme
 */
function theme_calendar_systems_languages_overview_form($form) {
  // Load helpers.
  module_load_include('inc', 'calendar_systems');

  $rows = array();
  $default = language_default();

  // Set form table rows.
  foreach ($form['name'] as $name => $element) {
    // Skip form attributes.
    if (!is_array($element) || !element_child($name)) {
      continue;
    }

    // Make the default language checkbox disabled.
    if ($name == $default->language) {
      $form['enabled'][$name]['#attributes']['disabled'] = 'disabled';
    }

    // Set form table header.
    $rows[] = array(
      array(
        'align' => 'center',
        'data'  => drupal_render($form['enabled'][$name]),
      ),
      check_plain($name),
      '<strong>' . drupal_render($form['name'][$name]) . '</strong>',
      drupal_render($form['native'][$name]),
      drupal_render($form['direction'][$name]),
      drupal_render($form['site_default'][$name]),
      drupal_render($form['admin_language'][$name]),
      drupal_render($form['calendar_systems_languages'][$name]),
      drupal_render($form['weight'][$name]),
      l(t('edit'), 'admin/settings/language/edit/'. $name) . (($name != 'en' && $name != $default->language) ? ' ' . l(t('delete'), 'admin/settings/language/delete/' . $name) : ''),
    );
  }

  // Set form table header.
  $header = array(
    t('Enabled'),
    t('Code'),
    t('English name'),
    t('Native name'),
    t('Direction'),
    t('Default'),
    t('Admin'),
    t('Calendar'),
    t('Weight'),
    array('data' => t('Operations')),
  );

  // Remove admin_language.module options if not exist.
  if (!isset($form['admin_language'][$name])) {
    unset($header[6]);
    $rows = array_map('array_filter', $rows);
  }
  // All; a waste of time!
  // TODO: Rewrite the logic and avoid this if possible.
  if (!_calendar_systems_patches_applied()) {
    unset($header[7]);
    // Temporary dirt:
    foreach ($rows as $index => $row) {
      unset($rows[$index][7]);
    }
  }

  // Let it go!
  $output = theme('table', $header, $rows);
  $output .= drupal_render($form);
  return $output . t('<a href="!link">Configure default calendar</a>', array(
    '!link' => url('admin/settings/date-time/calendars'),
  ));
}
