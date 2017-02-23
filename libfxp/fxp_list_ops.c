
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "fxp_private.h"

fxp_error_t fxp_expand_wildcards(fxp_handle_t fxp, const char *fragment, list_t *out_files)
{
  char *tmp;
  fxp_error_t fxpret;

  if(fxp == NULL || fragment == NULL || out_files == NULL)
    return -FXPENULLP;
  if(*fragment == 0x0 || strchr(fragment, '*') == NULL || fxp->ctrl_sock == 0)
    return -FXPEINVAL;

  if(!fxp->cant_expand_spaces && (fxpret= fxp_list_flags(fxp, fragment, out_files, "d")) != FXPOK)
      error2("first expansion method failed - [%s]\n", fxp_strerror(fxp, fxpret));
  if((tmp= strchr(fragment, '*')) != NULL && (tmp= strchr(tmp+1, '*')) != NULL) {
    error2("fragment contains multiple wildcards, skipping alternate expansion method and returning\n");
    return fxpret;
  }
  else if(fxpret != FXPOK || fxp->cant_expand_spaces) {
    char *dup= strdup(fragment), *first_space= strchr(dup, ' '), *first_star= strchr(dup, ' ');

    if(first_space != NULL) {
      int i;
      list_t *tmp_list= list_create();

      tmp= (first_star < first_space)? first_star: first_space;
      *tmp= '*';
      *(tmp+1)= 0x0;
      if((fxpret= fxp_list_flags(fxp, dup, tmp_list, "d")) != FXPOK) {
	error2("second expansion method failed, returning - [%s]\n", fxp_strerror(fxp, fxpret));
	list_destroy(tmp_list);
	free(dup);
	return fxpret;
      }

      debug("alternate expansion method worked!\n");
      fxp->cant_expand_spaces= 1;  //know this for sure now
      for(i=0; i<list_count(tmp_list); i++) {
	fxp_file_t *file= (fxp_file_t *)list_get(tmp_list, i);
	if(strncmp(file->name, dup, strlen(dup)-1) == 0)  //make sure it matches (without the '*')
	  list_add(out_files, file);
	else
	  free(file);
      }
      list_destroy_dontfreeelements(tmp_list);
    }
    else {
      error2("no spaces in fragment, skipping second method and returning\n");
      free(dup);
      return fxpret;
    }
    free(dup);
  }

  debug("fxp_expand_wildcards() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_nlst(fxp_handle_t fxp, const char *dirname, list_t *out_files)
{
  return fxp_nlst_flags(fxp, dirname, out_files, "-AF");
}

fxp_error_t fxp_nlst_flags(fxp_handle_t fxp, const char *dirname, list_t *out_files, const char *flags)
{
  unsigned codes[2], num_codes= sizeof(codes)/sizeof(codes[0]);
  char *dup;
  fxp_error_t fxpret;

  if(fxp == NULL || out_files == NULL)
    return -FXPENULLP;
  if((dirname != NULL && *dirname == 0x0) || fxp->ctrl_sock == 0 || fxp->file_open != 0)
    return -FXPEINVAL;

  if((fxpret= fxp_type(fxp, FXP_TYPE_ASCII)) != FXPOK && fxpret != -FXPENOCHANGE) {
    error2("fxp_type() failed, returning\n");
    return fxpret;
  }

  if((fxpret= fxp_data_establish_setup(fxp)) != FXPOK) {
    error2("fxp_data_establish_setup() failed, returning\n");
    return fxpret;
  }

  strncpy(fxp->buff, "NLST", sizeof(fxp->buff));
  if(flags != NULL && *flags != 0x0) {
    if(*flags == '-')
      snprintf(fxp->buff+strlen(fxp->buff), sizeof(fxp->buff)-strlen(fxp->buff), " %s", flags);
    else
      snprintf(fxp->buff+strlen(fxp->buff), sizeof(fxp->buff)-strlen(fxp->buff), " -%s", flags);
  }
  if(dirname != NULL)
    snprintf(fxp->buff+strlen(fxp->buff), sizeof(fxp->buff)-strlen(fxp->buff), " %s", dirname);
  strncat(fxp->buff, FXPEOL, sizeof(fxp->buff));

  dup= strdup(fxp->buff);
  if((fxpret= fxp_ctrl_send_message(fxp, dup)) != FXPOK) {
    free(dup);
    error2("fxp_ctrl_send_message(NLST) failed, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return fxpret;
  }
  free(dup);

  if((fxpret= fxp_data_establish(fxp)) != FXPOK) {
    if(fxpret != -FXPEINTERRUPT) {
      error2("fxp_data_establish() failed, returning\n");
      if(fxp_data_cleanup(fxp) != FXPOK)
	error2("fxp_data_cleanup() failed, ignoring\n");
      return fxpret;
    }
    debug("fxp_data_establish() interrupted, continuing()\n");
  }

  if((fxpret= fxp_ctrl_receive_codes(fxp, codes, &num_codes, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_codes() failed, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return fxpret;
  }
  if(codes[0] == 421) {
    error2("NLST 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(codes[0] == 550) {
    error2("NLST no files found, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPENOTFOUND;
  }
  if(codes[0] == 425) {
    error2("NLST couldn't build data connection, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPEUNKNOWN;
  }
  if(codes[0] != 150 && codes[0] != 125) {
    error2("NLST unknown code [%u], returning\n", codes[0]);
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPEUNKNOWN;
  }
  if(num_codes > 1 && codes[1] == 226) {  //wow, both codes gotten before transfer, that's ok
    fxp->last_xfer_code= codes[1];
    debug("NLST transfer already completed, nice\n");
  }
  debug("NLST succeeded\n");

  //not supposed to fail
  if((fxpret= fxp_data_receive_directory_list(fxp, out_files, 0)) != FXPOK) {
    error2("fxp_data_receive_directory_list() failed, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return fxpret;
  }

  if(fxp_data_cleanup(fxp) != FXPOK)
    error2("fxp_data_cleanup() failed, ignoring\n");

  if(num_codes < 2 || codes[1] != 226) {  //do we still need the success code?
    if((fxpret= fxp_ctrl_receive_code(fxp, codes, NULL)) != FXPOK) {
      error2("fxp_ctrl_receive_code() failed, returning\n");
      return fxpret;
    }
    if(codes[0] == 421) {
      error2("NLST 421, disconnecting and returning\n");
      if(fxp_disconnect_hard(fxp) != FXPOK)
	error2("fxp_disconnect_hard() failed, ignoring\n");
      return -FXPECLOSED;
    }
    if(codes[0] == 550) {
      error2("NLST no files found, returning\n");
      if(fxp_data_cleanup(fxp) != FXPOK)
	error2("fxp_data_cleanup() failed, ignoring\n");
      return -FXPENOTFOUND;
    }
    if(codes[0] != 226)
      error2("NLST unknown response [%u], ignoring\n", codes[0]);
  }

  debug("fxp_nlst_flags() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_list(fxp_handle_t fxp, const char *dirname, list_t *out_complete_files)
{
  return fxp_list_flags(fxp, dirname, out_complete_files, "");
}

fxp_error_t fxp_list_flags(fxp_handle_t fxp, const char *dirname, list_t *out_complete_files,
			   const char *flags)
{
  unsigned codes[2], num_codes= sizeof(codes)/sizeof(codes[0]);
  char *dup;
  fxp_error_t fxpret;

  if(fxp == NULL || out_complete_files == NULL)
    return -FXPENULLP;
  if((dirname != NULL && *dirname == 0x0) || fxp->ctrl_sock == 0 || fxp->file_open != 0)
    return -FXPEINVAL;

  if((fxpret= fxp_type(fxp, FXP_TYPE_ASCII)) != FXPOK && fxpret != -FXPENOCHANGE) {
    error2("fxp_type() failed, returning\n");
    return fxpret;
  }

  if((fxpret= fxp_data_establish_setup(fxp)) != FXPOK) {
    error2("fxp_data_establish_setup() failed, returning\n");
    return fxpret;
  }

  strncpy(fxp->buff, "LIST", sizeof(fxp->buff));
  if(flags != NULL && *flags != 0x0) {
    if(*flags == '-')
      snprintf(fxp->buff+strlen(fxp->buff), sizeof(fxp->buff)-strlen(fxp->buff), " %s", flags);
    else
      snprintf(fxp->buff+strlen(fxp->buff), sizeof(fxp->buff)-strlen(fxp->buff), " -%s", flags);
  }
  if(dirname != NULL)
    snprintf(fxp->buff+strlen(fxp->buff), sizeof(fxp->buff)-strlen(fxp->buff), " %s", dirname);
  strncat(fxp->buff, FXPEOL, sizeof(fxp->buff));

  dup= strdup(fxp->buff);
  if((fxpret= fxp_ctrl_send_message(fxp, dup)) != FXPOK) {
    free(dup);
    error2("fxp_ctrl_send_message(LIST) failed, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return fxpret;
  }
  free(dup);

  if((fxpret= fxp_data_establish(fxp)) != FXPOK) {
    if(fxpret != -FXPEINTERRUPT) {
      error2("fxp_data_establish() failed, returning\n");
      if(fxp_data_cleanup(fxp) != FXPOK)
	error2("fxp_data_cleanup() failed, ignoring\n");
      return fxpret;
    }
    debug("fxp_data_establish() interrupted, continuing()\n");
  }

  if((fxpret= fxp_ctrl_receive_codes(fxp, codes, &num_codes, NULL)) != FXPOK) {
    error2("fxp_ctrl_receive_codes() failed, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return fxpret;
  }
  if(codes[0] == 421) {
    error2("LIST 421, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return -FXPECLOSED;
  }
  if(codes[0] == 550) {
    error2("LIST no files found, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPENOTFOUND;
  }
  if(codes[0] == 425) {
    error2("LIST couldn't build data connection, returning\n");
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, returning\n");
    return -FXPEUNKNOWN;
  }
  if(codes[0] != 150 && codes[0] != 125) {
    error2("LIST unknown code [%u], returning\n", codes[0]);
    if(fxp_data_cleanup(fxp) != FXPOK)
      error2("fxp_data_cleanup() failed, ignoring\n");
    return -FXPEUNKNOWN;
  }
  if(num_codes > 1 && codes[1] == 226) {  //wow, both codes gotten before transfer, that's ok
    fxp->last_xfer_code= codes[1];
    debug("LIST transfer already completed, nice\n");
  }
  debug("LIST succeeded\n");

  //not supposed to fail
  if((fxpret= fxp_data_receive_directory_list(fxp, out_complete_files, 1)) != FXPOK) {
    error2("fxp_data_receive_directory_list() failed, disconnecting and returning\n");
    if(fxp_disconnect_hard(fxp) != FXPOK)
      error2("fxp_disconnect_hard() failed, ignoring\n");
    return fxpret;
  }

  if(fxp_data_cleanup(fxp) != FXPOK)
    error2("fxp_data_cleanup() failed, ignoring\n");

  if(num_codes < 2 || codes[1] != 226) {  //do we still need the success code?
    if((fxpret= fxp_ctrl_receive_code(fxp, codes, NULL)) != FXPOK) {
      error2("fxp_ctrl_receive_code() failed, returning\n");
      return fxpret;
    }
    if(codes[0] == 421) {
      error2("LIST 421, disconnecting and returning\n");
      if(fxp_disconnect_hard(fxp) != FXPOK)
	error2("fxp_disconnect_hard() failed, ignoring\n");
      return -FXPECLOSED;
    }
    if(codes[0] == 550) {
      error2("LIST no files found, returning\n");
      if(fxp_data_cleanup(fxp) != FXPOK)
	error2("fxp_data_cleanup() failed, ignoring\n");
      return -FXPENOTFOUND;
    }
    if(codes[0] != 226)
      error2("LIST unknown response [%u], ignoring\n", codes[0]);
  }

  debug("fxp_list_flags() succeeded\n");
  return FXPOK;
}

fxp_error_t fxp_data_receive_directory_list(fxp_handle_t fxp, list_t *out_files, int is_long_list)
{
  char *ptr, *lines[20], *pfx= NULL;
  unsigned ptr_size, lines_size;
  const unsigned lines_orig_size= sizeof(lines)/sizeof(char *);
  fxp_error_t fxpret;

  if(fxp == NULL || out_files == NULL)
    return -FXPENULLP;
  if(fxp->ctrl_sock == 0 || fxp->file_open != 0 || fxp->data_sock == 0)
    return -FXPEINVAL;

  ptr= fxp->buff;
  ptr_size= sizeof(fxp->buff);
  while(1) {
    char *frag= NULL;
    int parse_failed= 0;

    if((fxpret= fxp_data_receive(fxp, ptr, &ptr_size)) != FXPOK && ptr == fxp->buff)  //done?
      break;
    ptr[fxpret == FXPOK? ptr_size: 0]= 0x0;  //null-cap the data receive
    ptr= fxp->buff;

    while(1) {
      int i;

      frag= NULL;
      lines_size= lines_orig_size;
      if(buff_to_lines(ptr, lines, &lines_size, &frag) || lines_size < 1) {
	parse_failed= fxpret != FXPOK;
	break;
      }

      for(i=0; i<lines_size; i++) {
	fxp_complete_file_t cfile= {};
	fxp_file_t *out_file;
	int len;

	if(strstr(lines[i], "total") == lines[i]) {  //use "total..." line to override type of listing
	  is_long_list= 1;
	  continue;
	}
	if(*lines[i] != 0x0 && lines[i][(len= strlen(lines[i]))-1] == ':') {  //prepend dir:
	  lines[i][len-1]= 0x0;
	  pfx= strdup(lines[i]);
	  continue;
	}

	//fxp_complete_file_t
	if(is_long_list) {
	  if(fxp_parse_stat_string(fxp, lines[i], &cfile) != FXPOK) {
	    error2("parse_stat_string() failed, skipping\n");
	    continue;
	  }
	}
	else if(list_count(out_files) < 1) {  //last attempt to assume long listing...
	  debug("trying to determine if listing is actually a long listing...\n");
	  is_long_list= fxp_parse_stat_string(fxp, lines[0], &cfile) == FXPOK;
	}

	//fxp_file_t
	if(!is_long_list) {
	  cfile.type= FXP_FILE_TYPE_REGULAR;
	  strncpy(cfile.name, lines[i], sizeof(cfile.name));
	  if((len= strlen(cfile.name)) > 0) {
	    switch(cfile.name[len-1]) {
	    case '/':
	      cfile.type= FXP_FILE_TYPE_DIRECTORY;
	      cfile.name[len-1]= 0x0;
	      break;
	    case '@':
	      cfile.type= FXP_FILE_TYPE_LINK;
	      cfile.name[len-1]= 0x0;
	      break;
	    case '*':
	      cfile.name[len-1]= 0x0;
	      break;
	    }
	  }
	}

	if(cfile.is_complete) {
	  out_file= malloc(sizeof(fxp_complete_file_t));
	  memcpy(out_file, &cfile, sizeof(fxp_complete_file_t));
	}
	else {
	  out_file= malloc(sizeof(fxp_file_t));
	  memcpy(out_file, &cfile, sizeof(fxp_file_t));
	}
	if(pfx != NULL) {
	  char *tmp= strdup(out_file->name);
	  snprintf(out_file->name, sizeof(out_file->name), "%s/%s", pfx, tmp);
	  free(tmp);
	}
	list_add(out_files, out_file);
      }

      if(!(lines_size >= lines_orig_size && (frag != NULL && *frag != 0x0)))  //done?
	break;

      ptr= frag;  //nope, still have a fragment to parse
    }

    if(parse_failed)
      break;

    ptr= fxp->buff;
    ptr_size= sizeof(fxp->buff);
    if(frag != NULL && *frag != 0x0) {  //prepend the possible frag
      ptr_size -= strlen(frag);
      if(ptr != frag)
	strcpy(ptr, frag);
      ptr += strlen(ptr);
    }
  }
  free(pfx);

  debug("fxp_data_receive_directory_list() succeeded\n");
  return FXPOK;
}
