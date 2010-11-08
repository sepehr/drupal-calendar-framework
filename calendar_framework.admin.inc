<?php
// $Id$

/**
 * @file
 * Implements necessary callbacks for Calendar Framework administration forms.
 */

/**
 * Form callback to list all available calendars and their configs.
 *
 * @return
 *   An array of form elements to be themed later on.
 *
 * @ingroup forms
 */
function calendar_framework_calendars_form() {
  $forms = $options = array();
  $calendars = calendar_framework_calendars();

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
    '#default_value' => variable_get('calendar_framework_default_calendar', 'gregorian'),
    '#options' => $options,
  );

  // Note language specific calendars.
  $form['calendar_framework_notice'] = array(
    '#type' => 'item',
    '#value' => t('<strong>Note</strong> that calendars are also <a href="!link">configurable per language</a> basis.',
      array('!link' => url('admin/settings/language'))
    ),
  );

  // Append the submit button.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Set as default calendar'),
  );

  return $form;
}


/**
 * Form submission callback.
 */
function calendar_framework_calendars_form_submit($form, &$form_state) {
  if ($form_state['values']['default_calendar']) {
    variable_set('calendar_framework_default_calendar', $form_state['values']['default_calendar']);
    drupal_set_message(t('System default calendar has been updated.'));
  }
}


/**
 * Calendar Framework admin form themer.
 *
 * @param $form
 *   Admin form array.
 */
function theme_calendar_framework_calendars_form($form) {
  $rows = array();

  // Theming site-wide options.
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
function calendar_framework_calendar_form(&$form_state, $identifier) {
  $calendar = calendar_framework_calendars('calendar', $identifier);

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
      array('@calendar' => calendar_framework_calendar_title($identifier))
    ));

    return $form;
  }

  drupal_not_found();
  exit();
}


/**
 * Form validation callback for a calendar admin settings.
 */
function calendar_framework_calendar_form_validate($form, &$form_state) {
  // Pass to calendar form validator.
  $function = $form_state['values']['calendar']['config callback'] . '_validate';
  if (function_exists($function)) {
    $function($form, $form_state);
  }
}


/**
 * Form submission callback for a calendar admin settings.
 */
function calendar_framework_calendar_form_submit($form, &$form_state) {
  $calendar = $form_state['values']['calendar'];

  // Saving necessary configs.
  unset($form_state['values']['op'],
    $form_state['values']['submit'],
    $form_state['values']['form_id'],
    $form_state['values']['calendar'],
    $form_state['values']['form_token'],
    $form_state['values']['form_build_id']
  );
  variable_set('calendar_framework_settings_' . $calendar['identifier'], $form_state['values']);

  drupal_set_message(t('Configuration options has been saved.'));
}

