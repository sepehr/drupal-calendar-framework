<?php
// $Id$

/**
 * @file
 * Calendar Framework helper functions which need to be used in various module files.
 */

/**
 * Helper function to check whether the patch
 * is applied to the patch target file or not.
 *
 * @return
 *   Boolean value.
 *
 * @todo Check whether the runkit extension is loaded to do a dirty magic.
 */
function _calendar_framework_patch_applied() {
  $patches = _calendar_framework_patch_info(TRUE);

  foreach ($patches as $identifier => $patch) {
    // Patch target relative path to Drupal base.
    $relative_path = $patch['target'];

    // Patch target absolute path.
    if (module_exists('absolute_path')) {
      global $absolute_path;
      $target = $absolute_path . '/' . $relative_path;
    }
    else {
      $target = realpath(drupal_get_path('module', 'system') . '/../../' . $relative_path);
    }

    // Applied?
    if (strpos(file_get_contents($target), $patch['fingerprint']) === FALSE) {
      return FALSE;
    }
  }

  return TRUE;
}


/**
 * Helper function to create patch records
 * for patch_manager.module once the module gets installed.
 *
 * @param $patch_filename
 *   The name of the patch file.
 * @param $patch_filepath
 *   The relative path of patch file.
 *
 * @see http://drupal.org/project/patch_manager
 */
function _calendar_framework_patch_manager_insert($patch_filename = FALSE, $patch_filepath = FALSE) {
  if (!module_exists('patch_manager')) {
    return;
  }

  global $user;
  $patches = _calendar_framework_patch_info();

  foreach ($patches as $identifier => $patch) {
    // Copy the patch to Drupal files directory,
    // This way the file could be seen by CCK
    // filefield. Sorry for the suppression load!
    $new_path = file_directory_path() . '/';
    if (!@copy($patch['path'] . $patch['name'], $new_path . $patch['name'])) {
      $t = get_t();
      drupal_set_message($t('You have <a href="!patchmanager">Patch Manger</a> installed but we could not copy the Calendar Framework patch to files directory. Please check directory permissions and reinstall the module.',
        array('!patchmanager' => url('admin/build/patch'))
      ), 'warning');

      return FALSE;
    }

    // Oh, I wish I could automate eth!
    // chmod($patch_info['target'], 0646);

    // Create the patch file.
    // TODO: Check for duplicates.
    $file = new stdClass();
    $file->list = 1;
    $file->uid = $user->uid;
    $file->timestamp = time();
    $file->filename = $patch['name'];
    $file->filesource = $patch['name'];
    $file->filemime = file_get_mimetype($patch['name']);
    $file->filepath = $new_path . $patch['name'];
    $file->filesize = filesize($new_path . $patch['name']);
    $file->status = FILE_STATUS_PERMANENT;

    // Save the patch file.
    // Oh, missing D7 file_save().
    drupal_write_record('files', $file);
    $file->fid = db_result(db_query("SELECT fid FROM {files} WHERE filepath = '%s'", $file->filepath));

    // Create the patch node.
    $patch_node = new stdClass();
    $patch_node->type = 'patch';
    $patch_node->language = 'en';
    $patch_node->uid = $user->uid;
    $patch_node->title = $patch['title'];
    $patch_node->body = $patch['description'];
    $patch_node->field_module[0]['value'] = $patch['target module'];
    if (!is_null($patch['issue']) && is_numeric($patch['issue'])) {
      $patch_node->field_drupal_issue[0]['value'] = $patch['issue'];
    }

    // Attach the patch file to the node.
    $patch_node->field_patch[0] = array(
      'list' => 1,
      'fid' => $file->fid,
      'title' => $file->filename,
      'filename' => $file->filename,
      'filepath' => $file->filepath,
      'filesize' => $file->filesize,
      'mimetype' => $file->filemime,
    );

    // Save the patch node.
    content_presave($patch_node);
    node_save($patch_node);
    content_insert($patch_node);
  }

  // Make 'em happy!
  return TRUE;
}


/**
 * Helper function to remove patch records.
 *
 * @see http://drupal.org/node/292151
 * @see http://drupal.org/project/patch_manager
 */
function _calendar_framework_patch_manager_delete() {
  $patches = _calendar_framework_patch_info();

  foreach ($patches as $identifier => $patch) {
    $path = file_directory_path() . '/' . $patch['name'];
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
function _calendar_framework_patch_info($req = FALSE) {
  $t = get_t();
  $required = $optional = array();

  // Required patches.
  $required['common.inc'] = array(
    'target' => 'includes/common.inc',
    'target module' => 'core',
    'target version' => '6.19',
    'name' => 'common.inc-format_date.d6.patch',
    'path' => drupal_get_path('module', 'calendar_framework') . '/patch/',
    'fingerprint' => 'foreach (module_implements(\'format_date\') as $module)',
    'description' => $t('Drupal core common.inc expansion to support hook_format_date() implementations.'),
    'title' => $t('Calendar Framework common.inc patch for Drupal core'),
    'issue' => NULL,
  );

  // Optional patches.
  $optional['uc_deliverytimeslot'] = array(
    'target' => drupal_get_path('module', 'uc_deliverytimeslot') . '/uc_deliverytimeslot.module',
    'target module' => 'uc_deliverytimeslot',
    'target version' => '6.x-1.0-beta4',
    'name' => 'uc_deliverytimeslot-format_date.d6.patch',
    'path' => drupal_get_path('module', 'calendar_framework') . '/patch/',
    'fingerprint' => '$row[] = array(\'data\' => t(\'!day\', array(\'!day\' => format_date($timestamps, \'custom\', \'l\'))),',
    'description' => $t('Ubercart Delivery Timeslot module to use format_date() and support for calendar_framework.module.'),
    'title' => $t('Calendar Framework patch for Ubercart Delivery Timeslot module'),
    'issue' => '970768',
  );

  if ($req) {
    return $required;
  }
  else {
    return $required + $optional;
  }
}

