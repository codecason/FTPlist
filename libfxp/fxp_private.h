
#ifndef FXP_PRIVATE_H
#define FXP_PRIVATE_H


#include <fxp.h>

#define FXPEOL      "\r\n"
#define FXPEOL_LEN  2

typedef struct fxp_s {
  url_t url;                        //the connection's url (from fxp_init())

  int ctrl_sock;                    //the control connection socket (0 if no socket yet)
  fxp_send_hook ctrl_send_hook;     //control connection send callback (NULL if not set)
  fxp_receive_hook ctrl_recv_hook;  //control connection receive callback (NULL if not set)
  unsigned long ctrl_recv_timeo;    //timeout for control connection receives (0 for indefinite)

  int data_sock;                    //the data connection socket (0 if no socket yet)
  fxp_type_t data_type;             //the data transfer type (0 if not yet set)
  fxp_mode_t data_mode;             //the data connection mode (0 if not yet set)
  unsigned long data_estab_timeo;   //timeout for data connection establishment (0 for indefinite)
  unsigned long data_recv_timeo;    //timeout for data connection receives (0 for indefinite)
  unsigned long data_send_timeo;    //timeout for data connection sends (0 for indefinite)
  int data_conn_setup;              //1 if _establish_setup() was called but not _data_establish()
  unsigned short data_port;         //the data port number (host-byte order, 0 if not yet set)
  int data_failed_timing_out;       //set to 1 if data estab failed and the server is timing out 

  char *data_pasv_host;             //the passive data connection hostname (NULL if not yet determined)
  int data_actv_listen_sock;        //the active connection listening socket (0 if no listening socket)

  int file_open;                    //1 if a remote file is open for reading, 2 for writing (exclusive)
  int last_xfer_code;               //set to last code gotten during a data transfer or 0
  unsigned long long retr_size;     //set to last remote file size from _retr() or _retr_rest()
  int list_parse_method;            //indicates which directory list parse method to use (0 if unknown)
  int cant_expand_spaces;           //1 if can't expand wildcarded fragments containing spaces

  char *pwd_cache;                  //what it says (NULL if pwd not gotten yet)
  char buff[BUFFSZ];                //general-purpose buffer for internal use
} fxp_t;

//splits a buffer delimited by "\r\n" into out_lines.  fragment gets set to the possible trailing line
fxp_error_t buff_to_lines(char *buff, char **out_lines, int *out_lines_size, char **out_fragment);
//parses a code from the buffer
fxp_error_t line_to_code(const char *line, unsigned *out_code, const char **out_rest);
//parses stat string into fxp_complete_file_t
fxp_error_t fxp_parse_stat_string(fxp_handle_t fxp, const char *stat_string,
				  fxp_complete_file_t *out_complete_file);

//expands remote filenames given a fragment containing wildcards ('*'s)
fxp_error_t fxp_expand_wildcards(fxp_handle_t fxp, const char *fragment, list_t *out_files);

//disconnects the connection(s) ungracefully (doesn't send QUIT, etc)
fxp_error_t fxp_disconnect_hard(fxp_handle_t fxp);

//sends a control message to the server
fxp_error_t fxp_ctrl_send_message(fxp_handle_t fxp, const char *format, ...);
//receives a control response from the server
fxp_error_t fxp_ctrl_receive(fxp_handle_t fxp, char *out_buff, unsigned *out_buff_size);
//calls fxp_ctrl_receive_codes() to receive a single code
fxp_error_t fxp_ctrl_receive_code(fxp_handle_t fxp, unsigned *out_code, char **out_rest);
//sets a low timeout and calls fxp_ctrl_receive_code()
fxp_error_t fxp_ctrl_receive_code_quick(fxp_handle_t fxp, unsigned *out_code, char **out_rest);
//receives as many codes as possible without blocking, up to out_codes_size
fxp_error_t fxp_ctrl_receive_codes(fxp_handle_t fxp, unsigned *out_codes, unsigned *out_codes_size,
				   char **out_rest);

//performs data connection setup - call before _establish()
fxp_error_t fxp_data_establish_setup(fxp_handle_t fxp);
//establishes a data connection of the mode set with fxp_mode()
fxp_error_t fxp_data_establish(fxp_handle_t fxp);
//closes the data connection if still connected and performs cleanup
fxp_error_t fxp_data_cleanup(fxp_handle_t fxp);
//receives a chunk of data connection data
fxp_error_t fxp_data_receive(fxp_handle_t fxp, char *out_buff, unsigned *out_buff_size);
//sends a chunk of data connection data
fxp_error_t fxp_data_send(fxp_handle_t fxp, const char *buff, unsigned buff_size);
//receives (from data conn) and parses directory listing into a list of fxp_[complete_]file_t's
fxp_error_t fxp_data_receive_directory_list(fxp_handle_t fxp, list_t *out_files, int is_long_list);

//restarts the subsequent transfer from the given byte offset
fxp_error_t fxp_rest(fxp_handle_t fxp, unsigned long offset);

//lists files in the specified directory with given flags (or current directory if dirname == NULL)
fxp_error_t fxp_nlst_flags(fxp_handle_t fxp, const char *dirname, list_t *out_files,
			   const char *flags);
//lists files in the specified directory with given flags (or current directory if dirname == NULL)
fxp_error_t fxp_list_flags(fxp_handle_t fxp, const char *dirname, list_t *out_complete_files,
			   const char *flags);


#endif
