<?php 


function linkaccess(){
	$output = '';

	$temp2 = drupal_get_form('get_user_list_form');
	$output .= drupal_render($temp2);
	
	return $output;
}


function get_user_list_form($form, &$form_state){ 
	global $user;
	$options = array();
	$max_rid = max(array_keys($user->roles)); 
	$temp = db_query("SELECT rid, name FROM role WHERE weight > (SELECT weight FROM role WHERE rid='$max_rid') ORDER BY weight ASC")->fetchAll();
	if(!empty($temp) && is_array($temp)){
		foreach($temp AS $k=>$v){
			$options[$v->rid] = $v->name;
		}
	}

	$form = array();
	
	$form['fieldset'] = array(
		'#type'=>'fieldset',
		'#title'=>'Choose User',
	);
	
	$form['fieldset']['roles'] = array(
		'#type'=>'select',
		'#title'=>t('Role'),
		'#options'=>$options,
		'#default_value' => '',
		'#ajax' => array(
			'callback' => 'filter_user_by_role',
			'wrapper' => 'user_list_cont',
			'method' => 'replace'
		)
	);

	if(isset($form_state['values']['roles']) && is_numeric($form_state['values']['roles'])){
		
		$role = $form_state['values']['roles'];
		$users = get_users_by_role($role);		
		$form['fieldset']['user'] = array(
			'#type'=>'select',
			'#title'=>t('User'),
			'#options'=>$users,
			'#prefix' => '<div class="user_list_cont">',
			'#suffix' => '</div>',			
			'#required' => TRUE,
		);
	}
	else{
		
		$users = get_all_active_users();		
		
		$form['fieldset']['user'] = array(
			'#type'=>'select',
			'#title'=>t('Users'),
			'#options'=>$users,
			'#prefix' => '<div class="user_list_cont">',
			'#suffix' => '</div>',			
			'#required' => TRUE,
		);
	}

	$form['permission_form'] = get_permission_form();
	
	$form['filter'] = array(
		'#type'=>'submit',
		'#value'=>t('Apply Permissions'),
	);
	return $form;

}

function filter_user_by_role($form, $form_state){
	$commands = array();
	$commands[] = ajax_command_replace('.user_list_cont', drupal_render($form['fieldset']['user']));
	return array(
		'#type'=>'ajax',
		'#commands' => $commands
	);
}

function get_user_list_form_validate($form, $form_state){
	if(!empty($form_state['values']['user']) && $form_state['values']['user'] == '_none'){ 
		form_set_error($form['fieldset']['user'], t('Choose an user'));
	}
}

function get_user_list_form_submit($form, $form_state){ 
	/*echo '<pre>'; print_r($form_state['values']); echo '</pre>'; exit;*/
	$fv = $form_state['values'];
	if(is_numeric($fv['user'])){
		/* module access applying */
		$fields = db_query("SELECT type_name FROM module_links WHERE status='1'")->fetchAll();
		if(!empty($fields) && is_array($fields)){
			foreach($fields AS $f){
				$aflag = '0';
				if($fv[$f->type_name] == 'yes'){
					$aflag = '1';
				}
				$id = db_query("SELECT id FROM access_links WHERE uid = '".$fv['user']."' AND node_type = '".$f->type_name."'")->fetchField();
				if($id){
					db_update('access_links')
						->fields(array('aflag'=> $aflag))
						->condition('uid',$fv['user'],'=')
						->condition('node_type',$f->type_name,'=')
						->execute();
				}
				else{
					db_insert('access_links')
						->fields(array('uid'=>$fv['user'],'aflag'=>$aflag, 'node_type'=>$f->type_name))
						->execute();
				}
			}
		}

		/* page access applying */
		$fields = db_query("SELECT id FROM page_links WHERE status='1'")->fetchAll();
		if(!empty($fields) && is_array($fields)){
			foreach($fields AS $f){
				$aflag = '0';
				if($fv['pagefield_'.$f->id] == 'yes'){
					$aflag = '1';
				}
				$id = db_query("SELECT id FROM access_page_link WHERE uid = '".$fv['user']."' AND page_id = '".$f->id."'")->fetchField();
				if($id){
					db_update('access_page_link')
						->fields(array('aflag'=> $aflag))
						->condition('uid',$fv['user'],'=')
						->condition('page_id',$f->id,'=')
						->execute();
				}
				else{
					db_insert('access_page_link')
						->fields(array('uid'=>$fv['user'],'aflag'=>$aflag, 'page_id'=>$f->id))
						->execute();
				}
			}
		}


	}
}

function get_permission_form(){
	$pf = array();
	$pf['#prefix'] = '<div class="form-elem-cont">'; 
	$pf['#suffix'] = '</div>';
	
	

	$fields = db_query("SELECT DISTINCT type_name, type_title FROM `module_links` WHERE status='1'")->fetchAll();
	if(!empty($fields) && is_array($fields)){
		
		$pf['fs1'] = array(
			'#type'=>'fieldset',
			'#attributes' => array('class'=>array('field-holder')),
			'#title' => t('Modules'),
		); 

		foreach($fields AS $f){
			$pf['fs1'][$f->type_name] = array(
				'#type'=>'select',
				'#title'=>t($f->type_title),
				'#options' => array('yes'=>'Yes','no'=>'No'),
				'#default_value'=>'yes',
			);
		}
	}

	

	$fields = db_query("SELECT DISTINCT id, page_title FROM `page_links` WHERE status='1'")->fetchAll();
	if(!empty($fields) && is_array($fields)){
		
		$pf['fs2'] = array(
			'#type'=>'fieldset',
			'#attributes' => array('class'=>array('field-holder')),
			'#title' => t('Pages'),
		);

		foreach($fields AS $f){
			$pf['fs2']['pagefield_'.$f->id] = array(
				'#type'=>'select',
				'#title'=>t($f->page_title),
				'#options' => array('yes'=>'Yes','no'=>'No'),
				'#default_value'=>'yes',
			);
		}
	}
	return $pf; 
}


/*** 
callback module restriction
*/
function form_access_module(){
	$opt = ''; 
	$temp = drupal_get_form('form_module_restriction');
	$opt .= drupal_render($temp);
	$opt .= get_module_restriction_list();
	return $opt;
}

function form_module_restriction(){
	$form = array(); 
	$form['content_title'] = array(
		'#type'=>'textfield',
		'#title'=>t('Link Title'),
		'#description' => t('Content title. Human readable name'),
		'#required' => TRUE,			
	);

	$form['content_type'] = array(
		'#type'=>'textfield',
		'#title'=>t('Content Type'),
		'#description' => t('Machine readable name'),
		'#required' => TRUE,			
	);

	
	$form['submit'] = array('#type'=>'submit','#value'=>'Submit');

	return $form;
}

function form_module_restriction_validate($form,&$form_state){
	if(empty($form_state['values']['content_type'])){
		form_set_error($form['content_type'], t('Enter content machine readable name'));
	}
	if(empty($form_state['values']['content_title'])){
		form_set_error($form['content_type'], t('Enter content machine readable name'));
	}
}

function form_module_restriction_submit($form,&$form_state){
	global $user; 
	$fv = $form_state['values'];
	$type = strtolower(trim($fv['content_type']));
	$type = str_replace(' ','_',$type);

	db_insert('module_links')
		->fields(
				array(
					'type_name' => $type,
					'type_title' => $fv['content_title']
				)
			)->execute();
}

function get_module_restriction_list(){
	$header = array(
		'sl' => 'Sl.',
		'title'=>'Content Title',
		'type' => 'Content type',
		'edit' => 'Edit',
		'delete' => 'Delete'
	);

	$data = db_select('module_links','lct')->fields('lct')->execute()->fetchAll();
	if(!empty($data) && is_array($data)){
		$c =0; 
		foreach($data AS $v){
			$c++;
			$rows[] = array($c, $v->type_title,$v->type_name,'<a href="">Edit</a>','<a href="">Delete</a>');
		}
	}	
	
	$variables = array();

  	$variables['header'] = $header;
	$variables['rows'] = $rows;
	
	$variables['attributes'] = array();
	$variables['caption'] = '';
	$variables['colgroups'] = array();
	$variables['sticky'] = FALSE;
	$variables['empty'] = t('There is no records yet');

	return theme_table($variables);
}




/*** 
callback for page restrictions.
*/
function form_access_pages(){
	$opt = ''; 
	$temp = drupal_get_form('form_access_page_restriction');
	$opt .= drupal_render($temp);
	$opt .= get_page_restriction_list();
	return $opt;
}

function form_access_page_restriction(){
	$form = array(); 
	$form['page_title'] = array(
		'#type'=>'textfield',
		'#title'=>t('Page title'),
		'#description' => t('Enter page title. This title will show at permission settings page for admin.'),
		'#required' => TRUE,			
	);

	$form['page_link'] = array(
		'#type'=>'textfield',
		'#title'=>t('Page Link'),
		'#description' => t('Enter machine readable link. Copy from browser URL'),
		'#required' => TRUE,			
	);

	$form['submit'] = array('#type'=>'submit','#value'=>'Submit');

	return $form;
}

function form_access_page_restriction_validate($form,&$form_state){
	if(empty($form_state['values']['page_title'])){
		form_set_error($form['page_title'], t('Enter page title.'));
	}
	if(empty($form_state['values']['page_link'])){
		form_set_error($form['page_link'], t('Enter page title'));
	}
}

function form_access_page_restriction_submit($form,&$form_state){
	global $user; 
	$fv = $form_state['values'];
	$page_link = strtolower(trim($fv['page_link']));
	
	db_insert('page_links')
		->fields(
				array(
					'page_title' => $fv['page_title'],
					'page_link' => $page_link
				)
			)->execute();
}

function get_page_restriction_list(){
	$header = array(
		'sl' => 'Sl.',
		'title'=>'Page Title',
		'type' => 'Page link',
		'edit' => 'Edit',
		'delete' => 'Delete'
	);

	$rows = array();
	$data = db_select('page_links','pl')->fields('pl')->execute()->fetchAll();
	if(!empty($data) && is_array($data)){
		$c =0; 
		foreach($data AS $v){
			$c++;
			$rows[] = array($c, $v->page_title,$v->page_link,'<a href="">Edit</a>','<a href="">Delete</a>');
		}
	}	
	
	$variables = array();

  $variables['header'] = $header;
	$variables['rows'] = $rows;
	
	$variables['attributes'] = array();
	$variables['caption'] = '';
	$variables['colgroups'] = array();
	$variables['sticky'] = FALSE;
	$variables['empty'] = t('There is no records yet');

	return theme_table($variables);
}



