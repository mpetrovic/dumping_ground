<?php

/*
 * @file
 * What this page does in so many words:
 * 
 * First off, I categorize these tables into 2 groups:
 * Data tables, which contain nothing but data and a number to id that data by.
 * Linkage tables, which link together several data tables. 
 * 
 * The idea is to put together a relational array, such that any 'foreign key' in
 * a linkage table will instead be a reference to a row from a data table. Well, any relevant 
 * foreign key. 
 * 
 * We're not literally using SQL's foreign keys, but that's essentially what they are. 
 * 
 * Here we go.
 * 1. Put together the data array
 * 2. Put together the linkage array. Substitute any FKs with a reference to the proper row from the data array
 * 3. We have our relational array structure.
 * 
 * 4. Run through the data array, looking for collisions and existing matches in the current table. 
 * 5. Update the database or the data array depending on the situation.
 *     Id doesn't exist: add a new one
 *     Id exists, but the titles are the same: update hint
 *     Id exists, but title is different: collision!
 *       Title exists with different Id: use that Id
 *       Title doesn't exist: get new Id, use that, insert
 * 
 * 6. We took care of the collisions, and the linkage array knows about the updated ids. We should just be able
 *       to insert new rows and update existing ones without worrying about breaking linkages.
 * 
 * 7. Run through the linkage table. Update it if exists, insert if it doesn't.
 */

if (!function_exists('scholar_publications_biblio_config_features_default_settings'))
  include_once('scholar_publications.features.inc');
  
$config = scholar_publications_biblio_config_features_default_settings();

// take data tables and construct them

$field_type_data = array();

foreach ($config['biblio_field_type_data'] as $v) {
  $field_type_data[$v['ftdid']] = $v;
}

// take linkage tables and construct them with linkage to data tables

$field_type = array();

foreach ($config['biblio_field_type'] as $v) {
  $v['ftdid'] = &$field_type_data[$v['ftdid']];
  $v['cust_tdid'] = &$field_type_data[$v['cust_tdid']];
  $field_type = $v;
}

// add data tables to db, checking for collisions and updating self appropriately

foreach ($field_type_data as &$v) {
  $table = 'biblio_field_type_data_temp';
  
  $title = db_result(db_query("SELECT title FROM { $table } WHERE ftdid = %d", $v['ftdid']));
  if ($title === FALSE) {
    // this ftdid doesnt exist. its safe to add it.
    db_query("INSERT INTO { $table } VALUES (%d, '%s', '%s')", $v['ftdid'], $v['title'], $v['hint']);
  }
  elseif ($title == $v['title']) {
    // we're good to go. Update the hint and be done
    db_query("UPDATE { $table } SET hint = '%s' WHERE ftdid = %d", $v['hint'], $v['ftdid']);
  }
  else {
    // oh god collision!
    // look for an existing name
    
    $ftdid = db_result(db_query("SELECT ftdid FROM { table } WHERE title = '%s'", $v['ftdid']));
    if ($ftdid !== FALSE) {
      // we found the name with a different id
      $v['ftdid'] = $ftdid;
    }
    else {
      // the name doesn't exist and the ftdid is taken. add a new row entirely
      // why doesn't this table have auto increment if it's going to use unique integer fields?!
      $new_id = db_result(db_query("SELECT MAX(ftdid)+1 FROM { $table }"));
      $v['ftdid'] = $new_id;
      db_query("INSERT INTO { $table } VALUES (%d, '%s', '%s')", $v['ftdid'], $v['title'], $v['hint']);
    }
  }
}

// add linkage tables to db, checking for collisions and getting info from data tables as necessary

// by this point, we know whether an ftdid points to the correct resource or not.
// any matches we find for ftdid in the linkage table points to what we want it to, for sure.
foreach ($field_type as $v) {
  $table = 'biblio_field_type_temp';
  
  $act = array();
  foreach ($v as $k => $d) {
    if (is_array($d)) {
      $act[$k] = $d['ftdid'];
    }
    else {
      $act[$k] = $d;
    }
  }
  
  // field_type is something we can bulldoze. It's settings for form displays and layouts, not actual content
  $count = db_result(db_query("SELECT COUNT(*) FROM { $table } WHERE tid = %d AND fid = %d", $act['tid'], $act['fid']));
  if ($count > 0) {
    // row exists already. just update it
    db_query("UPDATE { $table } SET ftdid = %d, cust_tdid = %d, common = %d, autocomplete = %d, required = %d, weight = %d, visible = %d WHERE tid = %d AND fid = %d AND ftdid = %d", $act['ftdid'], $act['cust_tdid'], $act['common'], $act['autocomplete'], $act['required'], $act['weight'], $act['visible'], $act['tid'], $act['fid']);
  }
  else {
    db_query("INSERT INTO { $table } VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d)", $act['tid'], $act['fid'], $act['ftdid'], $act['cust_tdid'], $act['common'], $act['autocomplete'], $act['required'], $act['weight'], $act['visible']);
  }
}


// contributor type 

$contributor_type_data = array();

foreach ($config['biblio_contributor_type_data'] as $v) {
  $contributor_type_data[$v['auth_type']] = $v;
}

$contributor_type = array();

foreach ($config['biblio_contributor_type'] as $v) {
  $v['auth_type'] = &$contributor_type_data[$v['auth_type']];
  $contributor_type[] = $v;
}

foreach ($contributor_type_data as $v) {

  $table = 'biblio_contributor_type_data_temp';
  
  $title = db_result(db_query("SELECT title FROM { $table } WHERE auth_type = %d", $v['auth_type']));
  if ($title === FALSE) {
    // this auth_type doesnt exist. its safe to add it.
    db_query("INSERT INTO { $table } VALUES (%d, '%s', '%s')", $v['auth_type'], $v['title'], $v['hint']);
  }
  elseif ($title == $v['title']) {
    // we're good to go. Update the hint and be done
    db_query("UPDATE { $table } SET hint = '%s' WHERE auth_type = %d", $v['hint'], $v['auth_type']);
  }
  else {
    // oh god collision!
    // look for an existing name
    
    $auth_type = db_result(db_query("SELECT auth_type FROM { table } WHERE title = '%s'", $v['auth_type']));
    if ($auth_type !== FALSE) {
      // we found the name with a different id
      $v['auth_type'] = $auth_type;
    }
    else {
      // the name doesn't exist and the auth_type is taken. add a new row entirely
      // this table has an auto increment. let's use it
      if (@db_query("INSERT INTO { $table } VALUES (0, '%s', '%s')", $v['auth_type'], $v['title'], $v['hint'])) {
        $v['auth_type'] = db_last_insert_id($table, 'auth_type');
      }
    }
  }
}

foreach ($contributor_type as $v) {
  $table = 'biblio_contributor_type_temp';
  
  $act = array();
  foreach ($v as $k => $d) {
    if (is_array($d)) {
      $act[$k] = $d['auth_type'];
    }
    else {
      $act[$k] = $d;
    }
  }
  
  // contributor type isnt something we can just bulldoze. It's linked to by contributor, which is content
  $count = db_result(db_query("SELECT COUNT(*) FROM { $table } WHERE auth_category = %d AND biblio_type = %d AND auth_type = %d", $act['auth_category'], $ct['biblio_type'], $act['auth_type']));
  if ($count === 0) {
    // row doesn't exist. there's no additional data so just insert it as is
    db_query("INSERT INTO { $table } VALUES (%d, %d, %d)", $act['auth_category'], $ct['biblio_type'], $act['auth_type']);
  }
}