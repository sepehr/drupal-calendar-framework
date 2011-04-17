<?php

/**
 * @file
 * A collection of internal helpers around the compatibility patches.
 *
 * @todo
 *   - Break out the patch wrapper code into its own separate module, so we can
 *     focus on the real Calendar Systems module.
 */

/**
 * Internal helper to check whether the required patches are currently applied.
 *
 * @return
 *   TRUE if all required patches are applies, FALSE otherwise.
 */
function _calendar_systems_patches_applied() {
  // Get required patch arrays.
  $patches = _calendar_systems_patches_info(TRUE);

  foreach ($patches as $identifier => $patch) {
    // Findout patch target absolute path.
    // $patch['target'] should have its relative path to Drupal base.
    $target = function_exists('absolute_path') ?
      absolute_path() . '/' . $patch['target'] :
      realpath(drupal_get_path('module', 'system') . '/../../' . $patch['target']);

    // Applied?
    if (strpos(file_get_contents($target), $patch['fingerprint']) === FALSE) {
      return FALSE;
    }
  }

  return TRUE;
}


/**
 * Internal helper to create patch records for the lazy patch_manager module.
 *
 * Saves a "patch" typed node as per required by patch_manager.module once the
 * Calendar Systems gets installed.
 *
 * @param $patch_filename
 *   The name of the patch file.
 * @param $patch_filepath
 *   The relative path to the patch file.
 *
 * @return
 *   Boolean value indicating the process result.
 *
 * @see http://drupal.org/project/patch_manager
 *
 * @todo
 *   - Watch for duplicate patch introductions!
 */
function _calendar_systems_patches_pm_insert($patch_filename = FALSE, $patch_filepath = FALSE) {
  if (!module_exists('patch_manager')) {
    return;
  }

  global $user;
  // Get all required and optional patch arrays.
  $patches = _calendar_systems_patches_info();
  // Create and save a node per patch array.
  foreach ($patches as $identifier => $patch) {
    // Copy the patch to Drupal's files directory, This way the file
    // can be seen by CCK filefield, sorry for the suppression load!
    $new_path = file_directory_path() . '/';
    if (!@copy($patch['path'] . $patch['name'], $new_path . $patch['name'])) {
      // #FAIL.
      return FALSE;
    }

    // Oh, I wish I could automate eth!
    // chmod($patch['target'], 0646);

    // Create and save the patch file.
    $file = (object) array(
      'file' => 1,
      'uid'  => $user->uid,
      'timestamp' => time(),
      'filename'  => $patch['name'],
      'filesource' => $patch['name'],
      'filemime' => file_get_mimetype($patch['name']),
      'filepath' => $new_path . $patch['name'],
      'filesize' => filesize($new_path . $patch['name']),
      'status' => FILE_STATUS_PERMANENT,
    );
    // Oh, missing D7 file_save().
    drupal_write_record('files', $file);
    // Retrive the fid of the inserted file to be used in the "patch" typed node.
    $file->fid = db_result(db_query("SELECT fid FROM {files} WHERE filepath = '%s'", $file->filepath));

    // Build the patch node.
    $node = new stdClass();
    $node->type = 'patch';
    $node->language = 'en';
    $node->uid = $user->uid;
    $node->title = $patch['title'];
    $node->body = $patch['description'];
    // CCK module field:
    $node->field_module[0]['value'] = $patch['target module'];
    // CCK issue number field:
    if (!is_null($patch['issue']) && is_numeric($patch['issue'])) {
      $node->field_drupal_issue[0]['value'] = $patch['issue'];
    }

    // Attach the patch file to the node.
    $node->field_patch[0] = array(
      'list' => 1,
      'fid'  => $file->fid,
      'title' => $file->filename,
      'filename' => $file->filename,
      'filepath' => $file->filepath,
      'filesize' => $file->filesize,
      'mimetype' => $file->filemime,
    );

    // Save the patch node.
    content_presave($node);
    node_save($node);
    content_insert($node);
  }
  // Yo Happy?
  return TRUE;
}


/**
 * Internal helper to remove a patch_manager.module patch record.
 *
 * @see http://drupal.org/node/292151
 * @see http://drupal.org/project/patch_manager
 */
function _calendar_systems_patches_pm_delete() {
  if (!module_exists('patch_manager')) {
    return;
  }

  // Get all available patches.
  $patches = _calendar_systems_patches_info();
  // Delete all corresponding "patch" typed nodes.
  foreach ($patches as $identifier => $patch) {
    $path  = file_directory_path() . '/' . $patch['name'];
    $query = "SELECT ctp.nid
              FROM {files} f
              LEFT JOIN {content_type_patch} ctp ON f.fid = ctp.field_patch_fid
              WHERE f.filepath = '%s'";
    $nid = db_result(db_query($query, $path));
    node_delete($nid);
  }
}


/**
 * Helper function to return patches information.
 *
 * @param $req
 *   Imply whether to return all patches information or just the required ones.
 *
 * @return
 *   Either partial or full patch file info.
 */
function _calendar_systems_patch_info($req = FALSE) {
  $t = get_t();
  $required = $optional = array();

  // Required patches.
  $required['common.inc'] = array(
    'target' => 'includes/common.inc',
    'target module' => 'core',
    'target version' => '6.19',
    'name' => 'common.inc-format_date.d6.patch',
    'path' => drupal_get_path('module', 'calendar_systems') . '/patch/',
    'fingerprint' => 'foreach (module_implements(\'format_date\') as $module)',
    'description' => $t('Drupal core common.inc expansion to support hook_format_date() implementations.'),
    'title' => $t('Calendar Systems common.inc patch for Drupal core'),
    'issue' => NULL,
  );

  // Optional patches.
  $optional['uc_deliverytimeslot'] = array(
    'target' => drupal_get_path('module', 'uc_deliverytimeslot') . '/uc_deliverytimeslot.module',
    'target module' => 'uc_deliverytimeslot',
    'target version' => '6.x-1.0-beta4',
    'name' => 'uc_deliverytimeslot-format_date.d6.patch',
    'path' => drupal_get_path('module', 'calendar_systems') . '/patch/',
    'fingerprint' => '$row[] = array(\'data\' => t(\'!day\', array(\'!day\' => format_date($timestamps, \'custom\', \'l\'))),',
    'description' => $t('Ubercart Delivery Timeslot module to use format_date() and support for calendar_systems.module.'),
    'title' => $t('Calendar Systems patch for Ubercart Delivery Timeslot module'),
    'issue' => '970768',
  );

  return $req ? $required : $required + $optional;
}
