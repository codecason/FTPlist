
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fxp_private.h"

const char *fxp_strerror(fxp_handle_t fxp, fxp_error_t error)
{
  if(fxp == NULL)
    return NULL;

  switch(error) {
  case FXPOK:
    strncpy(fxp->buff, "operation successful", sizeof(fxp->buff));
    break;
  case FXPEUNKNOWN:
    strncpy(fxp->buff, "unknown error", sizeof(fxp->buff));
    break;
  case FXPETIMEOUT:
    strncpy(fxp->buff, "operation timed out", sizeof(fxp->buff));
    break;
  case FXPEACCESS:
    strncpy(fxp->buff, "access denied", sizeof(fxp->buff));
    break;
  case FXPENOTFOUND:
    strncpy(fxp->buff, "not found", sizeof(fxp->buff));
    break;
  case FXPEMEMORY:
    strncpy(fxp->buff, "error allocating memory", sizeof(fxp->buff));
    break;
  case FXPENOCHANGE:
    strncpy(fxp->buff, "operation's completion wouldn't have changed anything", sizeof(fxp->buff));
    break;
  case FXPENULLP:
    strncpy(fxp->buff, "NULL pointer encountered (check parameters)", sizeof(fxp->buff));
    break;
  case FXPEINVAL:
    strncpy(fxp->buff, "invalid value encountered (check parameters", sizeof(fxp->buff));
    break;
  case FXPECLOSED:
    strncpy(fxp->buff, "connection closed", sizeof(fxp->buff));
    break;
  case FXPEUNSUPP:
    strncpy(fxp->buff, "operation unsupported", sizeof(fxp->buff));
    break;
  default:
    strncpy(fxp->buff, "error code unknown", sizeof(fxp->buff));
  }

  return fxp->buff;
}

fxp_error_t buff_to_lines(char *buff, char **out_lines, int *out_lines_size, char **out_fragment)
{
  char *tmp;
  int idx= 0;

  if(buff == NULL || out_lines == NULL || out_lines_size == NULL)
    return -FXPENULLP;

  if(out_fragment != NULL)
    *out_fragment= NULL;

  while(1) {
    if(idx >= *out_lines_size)  //not enough room
      break;

    if(*buff == 0x0)  //done.
      break;

    if((tmp= strstr(buff, FXPEOL)) == NULL)  //no more delims
      break;

    memset(tmp, 0x0, FXPEOL_LEN);  //replace delim with 0x0's
    while(*buff != 0x0 && (*buff == ' ' || *buff == '\t'))
      buff++;

    if(*buff == 0x0)
      goto buff_to_lines_next;  //oops, line's empty now, oh well

    out_lines[idx++]= buff;

  buff_to_lines_next:
    buff= tmp+FXPEOL_LEN;
  }

  if(out_fragment != NULL)
    *out_fragment= (*buff == 0x0)? NULL: buff;
  *out_lines_size= idx;

  return FXPOK;
}

fxp_error_t line_to_code(const char *line, unsigned *out_code, const char **out_rest)
{
  const char *cptr, *csave= NULL;
  char smbuff[SMBUFFSZ];

  if(line == NULL || out_code == NULL)
    return -FXPENULLP;

  while(*line == ' ' || *line == '\t')  //strip leading whitespace
    if(*line++ == 0x0)
      return -FXPEINVAL;

  cptr= line;
  while(*cptr != 0x0 && (*cptr != ' ' && *cptr != '\t')) {
    if(*cptr < '0' || *cptr > '9')
      return -FXPEINVAL;  //encountered non-digit
    cptr++;
  }
  if(line == cptr)
    return -FXPEINVAL;
  if(*cptr != 0x0 && *(cptr+1) != 0x0)
    csave= cptr+1;

  strncpy(smbuff, line, cptr-line);
  *out_code= (unsigned)atol(smbuff);
  if(out_rest != NULL)
    *out_rest= csave;

  return FXPOK;
}

fxp_error_t parse_date_1(const char *month, const char *day, const char *timestr, time_t *out_time)
{
  struct tm tm= {};
  char *tok, *tok1, *ptr, *timedup;

  if(month == NULL || day == NULL || timestr == NULL || out_time == NULL)
    return -FXPENULLP;
  if(*month == 0x0 || *day == 0x0 || timestr == 0x0)
    return -FXPEINVAL;

  if(strncasecmp(month, "jan", 3) == 0)
    tm.tm_mon= 0;
  else if(strncasecmp(month, "feb", 3) == 0)
    tm.tm_mon= 1;
  else if(strncasecmp(month, "mar", 3) == 0)
    tm.tm_mon= 2;
  else if(strncasecmp(month, "apr", 3) == 0)
    tm.tm_mon= 3;
  else if(strncasecmp(month, "may", 3) == 0)
    tm.tm_mon= 4;
  else if(strncasecmp(month, "jun", 3) == 0)
    tm.tm_mon= 5;
  else if(strncasecmp(month, "jul", 3) == 0)
    tm.tm_mon= 6;
  else if(strncasecmp(month, "aug", 3) == 0)
    tm.tm_mon= 7;
  else if(strncasecmp(month, "sep", 3) == 0)
    tm.tm_mon= 8;
  else if(strncasecmp(month, "oct", 3) == 0)
    tm.tm_mon= 9;
  else if(strncasecmp(month, "nov", 3) == 0)
    tm.tm_mon= 10;
  else if(strncasecmp(month, "dec", 3) == 0)
    tm.tm_mon= 11;
  else
    debug("unknown month [%s]!\n", month);

  tm.tm_mday= atoi(day);

  timedup= strdup(timestr);
  if((tok= strtok_mw(timedup, ":", &ptr)) == NULL)
    debug("error parsing time [%s]\n", timestr);
  else if((tok1= strtok_mw(NULL, "", &ptr)) == NULL)
    tm.tm_year= atoi(tok)-1900;
  else {
    time_t cur_time= time(NULL);

    tm.tm_year= gmtime(&cur_time)->tm_year;
    tm.tm_hour= atoi(tok);
    tm.tm_min= atoi(tok1);
  }
  free(timedup);

  *out_time= mktime(&tm);

  return FXPOK;
}

fxp_error_t parse_date_2(const char *mmddyy_str_dashes, const char *timestr, time_t *out_time)
{
  struct tm tm= {};
  char *tok, *tok1, *ptr, *timedup;

  if(mmddyy_str_dashes == NULL || timestr == NULL || out_time == NULL)
    return -FXPENULLP;
  if(*mmddyy_str_dashes == 0x0 || *timestr == 0x0)
    return -FXPEINVAL;

  if(sscanf(mmddyy_str_dashes, "%d-%d-%d", &tm.tm_mon, &tm.tm_mday, &tm.tm_year) != 3) {
    debug("couldn't parse dash-delimited date, returning\n");
    return -FXPEINVAL;
  }

  timedup= strdup(timestr);
  if((tok= strtok_mw(timedup, ":", &ptr)) == NULL) {
    debug("error parsing time [%s]\n", timestr);
    free(timedup);
    return -FXPEINVAL;
  }
  else if((tok1= strtok_mw(NULL, "M", &ptr)) == NULL)
    tm.tm_year= atoi(tok)-1900;
  else {
    time_t cur_time= time(NULL);
    int len;

    len= strlen(tok1);
    switch(tok1[len-1]) {
    case 'P':
      tm.tm_hour += 12;
    case 'A':
      tok1[len-1]= 0x0;
    }

    tm.tm_year= gmtime(&cur_time)->tm_year;
    tm.tm_hour += atoi(tok);
    tm.tm_min= atoi(tok1);
  }
  free(timedup);

  if((*out_time= mktime(&tm)) == (time_t)-1)
    return -FXPEINVAL;

  return FXPOK;
}

fxp_error_t parse_access_1(const char *attribs, unsigned short *out_access)
{
  unsigned short access= 00;

  if(attribs == NULL || out_access == NULL)
    return -FXPENULLP;
  if(*attribs == 0x0 || strlen(attribs) < 3)
    return -FXPEINVAL;

  if(attribs[0] == 'r') access |= 0400;
  else                  access &= ~0400;
  if(attribs[1] == 'w') access |= 0200;
  else                  access &= ~0200;
  if(attribs[2] == 'x' || attribs[2] == 's') access |= 0100;
  else                  access &= ~0100;

  if(attribs[3] == 'r') access |= 0040;
  else                  access &= ~0040;
  if(attribs[4] == 'w') access |= 0020;
  else                  access &= ~0020;
  if(attribs[5] == 'x' || attribs[5] == 's') access |= 0010;
  else                  access &= ~0010;

  if(attribs[6] == 'r') access |= 0004;
  else                  access &= ~0004;
  if(attribs[7] == 'w') access |= 0002;
  else                  access &= ~0002;
  if(attribs[8] == 'x' || attribs[8] == 's') access |= 0001;
  else                  access &= ~0001;

  *out_access= access;

  return FXPOK;
}

fxp_error_t parse_stat_string_1(const char *stat_string, fxp_complete_file_t *out_complete_file)
{
  char *tok, *ptr, *statdup= NULL;
  int len;

  memset(out_complete_file, 0, sizeof(fxp_complete_file_t));
  statdup= strdup(stat_string);

  if((tok= strtok_mw(statdup, " \t", &ptr)) == NULL)
    goto parse_stat_string_error;

  if(strlen(tok) < 10) {  //parse attribs
    debug("stat string didn't seem to start with an attributes field, returning\n");
    goto parse_stat_string_error;
  }

  switch(tok[0]) {  //parse file type
  case 'd':
    out_complete_file->type= FXP_FILE_TYPE_DIRECTORY;
    break;
  case 'l':
    out_complete_file->type= FXP_FILE_TYPE_LINK;
    break;
  case 's':  //not sure if this is a valid response, but ls sez it's a socket
    out_complete_file->type= FXP_FILE_TYPE_SOCKET;
    break;
  case '-':  //file
  default:
    out_complete_file->type= FXP_FILE_TYPE_REGULAR;
  }

  parse_access_1(tok+1, &out_complete_file->access);

  if((tok= strtok_mw(NULL, " \t", &ptr)) == NULL) {  //parse number of hard links
    debug("stat string didn't seem to have a num-hard-links field, returning\n");
    goto parse_stat_string_error;
  }
  out_complete_file->num_hard_links= atoi(tok);

  if((tok= strtok_mw(NULL, " \t", &ptr)) == NULL) {  //parse owning user
    debug("stat string didn't seem to have an owner field, returning\n");
    goto parse_stat_string_error;
  }
  else {
    char *tok1, *tmp;

    if((tok1= strtok_mw(NULL, " \t", &ptr)) == NULL) {  //parse owning group or some number
      debug("stat string didn't seem to have a groupish field, returning\n");
      goto parse_stat_string_error;
    }

    if(strtol(ptr, &tmp, 10) == 0 && ptr == tmp) {
      debug("HACK: FUCKED UP LISTING, FIXME - dropping [%s]\n", tok);
      out_complete_file->file_size= atol(tok1);  //TODO: atoll()?
    }
    else {
      strncpy(out_complete_file->owning_user, tok, sizeof(out_complete_file->owning_user));
      strncpy(out_complete_file->owning_group, tok1, sizeof(out_complete_file->owning_group));

      if((tok= strtok_mw(NULL, " \t", &ptr)) == NULL) {  //parse file size
	debug("stat string didn't seem to have a file size field, returning\n");
	goto parse_stat_string_error;
      }
      sscanf(tok, "%llu", &out_complete_file->file_size);
    }
  }

  {  //parse timestamp and filename
    char timebuff[SMBUFFSZ], *extraptr;
    int monthidx, dayidx, timeidx;

    monthidx= 0;
    if((tok= strtok_mw(NULL, " \t", &ptr)) == NULL) {  //parse month
      debug("stat string didn't seem to have a month field, returning\n");
      goto parse_stat_string_error;
    }
    strncpy(timebuff + monthidx, tok, sizeof(timebuff) - monthidx);

    dayidx= monthidx + strlen(timebuff + monthidx)+1;
    if((tok= strtok_mw(NULL, " \t", &ptr)) == NULL) {  //parse day
      debug("stat string didn't seem to have a day field, returning\n");
      goto parse_stat_string_error;
    }
    strncpy(timebuff + dayidx, tok, sizeof(timebuff) - dayidx);

    timeidx= dayidx + strlen(timebuff + dayidx)+1;
    while(*ptr == ' ' || *ptr == '\t')
      ptr++;
    if(*ptr == 0x0) {
      debug("stat string didn't seem to have a year/time field, returning\n");
      goto parse_stat_string_error;
    }
    extraptr= ptr;
    while(*ptr != ' ' && *ptr != '\t') {
      if(*ptr == 0x0) {
	debug("stat string didn't seem to have a year/time field, returning\n");
	goto parse_stat_string_error;
      }
      ptr++;
    }
    *ptr++= 0x0;  //reached a whitespace
    strncpy(timebuff + timeidx, extraptr, sizeof(timebuff) - timeidx);

    parse_date_1(timebuff + monthidx, timebuff + dayidx, timebuff + timeidx,
		 &out_complete_file->timestamp);

    if((tok= strstr(ptr, " ->")) != NULL) //symbolic links do the 'blah -> realblah' thing
      *tok= 0x0;
    //everything after the first space after the hour:minute should be the filename
    strncpy(out_complete_file->name, ptr, sizeof(out_complete_file->name));  
    if((len= strlen(out_complete_file->name)) > 0) {
      switch(out_complete_file->name[len-1]) {
      case '/':
      case '@':
      case '*':
	out_complete_file->name[len-1]= 0x0;  //just discard sufficies (already know file type)
      }
    }
  }

  out_complete_file->is_complete= 1;
  strncpy(out_complete_file->raw_line, stat_string, sizeof(out_complete_file->raw_line));

  free(statdup);
  return 0;

 parse_stat_string_error:
  debug("parse_stat_string_1() failed, returning\n");
  free(statdup);
  return -1;
}

fxp_error_t parse_stat_string_2(const char *stat_string, fxp_complete_file_t *out_complete_file)
{
  char *tok, *tok1, *ptr, *statdup= NULL;
  int len;

  memset(out_complete_file, 0, sizeof(fxp_complete_file_t));
  statdup= strdup(stat_string);

  if((tok= strtok_mw(statdup, " \t", &ptr)) == NULL)
    goto parse_alt_stat_string_error;
  if((tok1= strtok_mw(NULL, " \t", &ptr)) == NULL)
    goto parse_alt_stat_string_error;
  if(parse_date_2(tok, tok1, &out_complete_file->timestamp) != FXPOK)
    goto parse_alt_stat_string_error;

  if((tok= strtok_mw(NULL, " \t", &ptr)) == NULL)
    goto parse_alt_stat_string_error;
  if(*tok == '<') {
    out_complete_file->type= FXP_FILE_TYPE_DIRECTORY;
    while(*ptr != 0x0 && (*ptr == ' ' || *ptr == '\t'))
      ptr++;
    if(*ptr == 0x0) 
      debug("directory had no name!\n");
  }
  else {
    out_complete_file->type= FXP_FILE_TYPE_REGULAR;
    sscanf(tok, "%llu", &out_complete_file->file_size);
    if(*ptr != 0x0)
      ptr++;
  }
  strncpy(out_complete_file->name, ptr, sizeof(out_complete_file->name));
  if((len= strlen(out_complete_file->name)) > 0) {
    switch(out_complete_file->name[len-1]) {
    case '/':
    case '@':
    case '*':
      out_complete_file->name[len-1]= 0x0;  //just discard sufficies (already know file type)
    }
  }

  free(statdup);

  out_complete_file->is_complete= 1;
  strncpy(out_complete_file->raw_line, stat_string, sizeof(out_complete_file->raw_line));

  return 0;

 parse_alt_stat_string_error:
  debug("parse_stat_string_2() failed, returning\n");
  free(statdup);
  return -1;
}

fxp_error_t fxp_parse_stat_string(fxp_handle_t fxp, const char *stat_string,
				  fxp_complete_file_t *out_complete_file)
{
  fxp_error_t fxpret;
  int cur_method= 0;

  if(fxp == NULL || stat_string == NULL || out_complete_file == NULL)
    return -FXPENULLP;
  if(*stat_string == 0x0)
    return -FXPEINVAL;

  while(*stat_string == ' ' || *stat_string == '\t' || *stat_string == '\n') {  //strip whitespace
    if(*stat_string == 0x0) {
      debug("stat_string was just whitespace!, returning\n");
      return -FXPEINVAL;
    }
    stat_string++;
  }
  if(fxp->list_parse_method != 0) {  //i seem to think i already know how to parse this one
    switch(fxp->list_parse_method) {
    case 1:
      fxpret= parse_stat_string_1(stat_string, out_complete_file);
      break;
    case 2:
      fxpret= parse_stat_string_2(stat_string, out_complete_file);
      break;
    }
    if(fxpret == FXPOK) {
      debug("parsed_%d stat_string [%s]\n", fxp->list_parse_method, stat_string);
      return fxpret;
    }
  }
  for(cur_method= 1; cur_method <= 2; cur_method++) {  //try all known methods
    if(fxp->list_parse_method != cur_method) {  //skip if i already tried
      switch(cur_method) {
      case 1:
	fxpret= parse_stat_string_1(stat_string, out_complete_file);
	break;
      case 2:
	fxpret= parse_stat_string_2(stat_string, out_complete_file);
	break;
      }
      if(fxpret == FXPOK) {
	fxp->list_parse_method= cur_method;
	debug("parsed_%d stat_string [%s]\n", fxp->list_parse_method, stat_string);
	return fxpret;
      }
    }
  }

  debug("all methods failed to parse [%s], returning\n", stat_string);
  memset(out_complete_file, 0, sizeof(fxp_complete_file_t));
  return -FXPEINVAL;
}
