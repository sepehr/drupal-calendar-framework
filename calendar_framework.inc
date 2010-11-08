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
  $patch_info = _calendar_framework_patch_info();

  // Patch target relative path to Drupal base.
  $relative_path = $patch_info['target'];

  // Patch target absolute path.
  if (module_exists('absolute_path')) {
    global $absolute_path;
    $target = $absolute_path . '/' . $relative_path;
  }
  else {
    $target = realpath(drupal_get_path('module', 'system') . '/../../' . $relative_path);
  }

  // Applied?
  // We won't get a negetive position ever.
  return (bool) strpos(file_get_contents($target), $patch_info['fingerprint']);
}


/**
 * Helper function to create a patch record
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
  $t = get_t();
  $patch_info = _calendar_framework_patch_info();

  // Copy the patch to Drupal files directory,
  // This way the file could be seen by CCK 
  // filefield. Sorry for the suppression load mess!
  $new_path = file_directory_path() . '/';
  if (!@copy($patch_info['path'] . $patch_info['name'], $new_path . $patch_info['name'])) {
    drupal_set_message($t('You have <a href="!patchmanager">Patch Manger</a> installed but we could not copy the Calendar Framework patch to files directory. Please check directory permissions and reinstall the module.',
      array(
        '!patchmanager' => url('admin/build/patch'),
      )
    ), 'warning');

    return FALSE;
  }

  // Oh, I wish I could automate eth!
  // chmod($patch_info['target'], 0646);

  // Create the patch file.
  $file = new stdClass();
  $file->list = 1;
  $file->uid = $user->uid;
  $file->timestamp = time();
  $file->filename = $patch_info['name'];
  $file->filesource = $patch_info['name'];
  $file->filemime = file_get_mimetype($patch_info['name']);
  $file->filepath = $new_path . $patch_info['name'];
  $file->filesize = filesize($new_path . $patch_info['name']);
  $file->status = FILE_STATUS_PERMANENT;

  // Save the patch file.
  // Oh, we miss D7 file_save().
  drupal_write_record('files', $file);
  $file->fid = db_result(db_query("SELECT fid FROM {files} WHERE filepath = '%s'", $file->filepath));
  
  // Create the patch node.
  $patch = new stdClass();
  $patch->type = 'patch';
  $patch->language = 'en';
  $patch->uid = $user->uid;
  $patch->title = $t('Calendar Framework common.inc patch for core');
  $patch->body = $t('Drupal core common.inc expansion to support hook_format_date() implementations.');
  $patch->field_module[0]['value'] = 'core';

  // Attach the patch file to the node.
  $patch->field_patch[0] = array(
		'fid' => $file->fid,
		'title' => $file->filename,
		'filename' => $file->filename,
		'filepath' => $file->filepath,
		'filesize' => $file->filesize,
		'mimetype' => $file->filemime,
		'list' => 1,
	);

  // Save the patch node.
  content_presave($patch); 
  node_save($patch);
  content_insert($patch);

  return TRUE;
}


/**
 * Helper function to remove the patch record.
 *
 * @see http://drupal.org/node/292151
 * @see http://drupal.org/project/patch_manager
 */
function _calendar_framework_patch_manager_delete() {
  $path = file_directory_path() . '/' . _calendar_framework_patch_info('name');
  $query = "SELECT ctp.nid
            FROM {files} f
            LEFT JOIN {content_type_patch} ctp ON f.fid = ctp.field_patch_fid
            WHERE f.filepath = '%s'";
  $nid = db_result(db_query($query, $path));
  node_delete($nid);   
}


/**
 * Helper function to return patch info.
 *
 * @param $type
 *   Patch info type to return.
 *   - all: Returns all information.
 *   - name: Returns patch file name.
 *   - path: Returns patch file path.
 *   - target: Returns patch file target.
 *   - finger: Returns patch fingerprint to check whether the patch is applied or not.
 *
 * @return
 *   Either partial or full Patch info.
 */
function _calendar_framework_patch_info($type = 'all') {
  $info = array(
    'target' => 'includes/common.inc',
    'name' => 'common.inc-format_date.d6.patch',
    'path' => drupal_get_path('module', 'calendar_framework') . '/patch/',
    'fingerprint' => 'foreach (module_implements(\'format_date\') as $module)',
  );

  switch ($type) {
    case 'all':
      return $info;

    default:
      return $info[$type];
  }
}

