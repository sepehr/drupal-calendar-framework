<?php
// $Id$

/**
 * @file
 * Implements necessary callbacks for Calendar Framework administration forms.
 */

/**
 * Form callback for listing all available calendars and their configs.
 *
 * @return
 *   An array of form elements.
 *
 * @ingroup forms
 */
function calendar_framework_calendars_form() {
  $forms = $options = array();
  $calendars = calendar_framework_calendars();
  
  foreach ($calendars as $identifier => $calendar) {
    $options[$identifier] = '';

    $form[$identifier]['id'] = array(
      '#value' => $identifier,
    );
    
    $form[$identifier]['config'] = array(
      '#value' => function_exists($calendar['admin form']) ? 
        l(t('Configuration'), 'admin/settings/date-time/calendars/' . $identifier) :
        t('No configuration'),
    );
  }
  
  $form['default'] = array(
    '#type' => '',
    '#default_value' => variable_get('calendar_framework_default_calendar', 'gregorian'),
    '#options' => $options,
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save as default'),
  );
  
  return $form;
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

}
