
#ifndef FXP_H
#define FXP_H


#include <millweed/parse.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  FXPOK            = 0,      //operation successful
  FXPEUNKNOWN      = 10,     //unknown error
  FXPETIMEOUT      = 20,     //operation timed out
  FXPEACCESS,                //access denied or authentication failed
  FXPENOTFOUND,              //requesting thing not found
  FXPEMEMORY,                //error allocating memory
  FXPENOCHANGE,              //operation's completion wouldn't have changed anything
  FXPENULLP,                 //NULL pointer encountered (check parameters)
  FXPEINVAL,                 //invalid value encountered (check parameters)
  FXPECLOSED,                //connection closed
  FXPEUNSUPP,                //operation unsupported
  FXPEINTERRUPT,             //operation was interrupted
} fxp_error_t;

typedef enum {
  FXP_TYPE_ASCII   = 1,
  FXP_TYPE_BINARY,
} fxp_type_t;                //transfer type

typedef enum {
  FXP_MODE_ACTIVE  = 1,
  FXP_MODE_PASSIVE,
} fxp_mode_t;                //data connection mode

typedef struct fxp_s * fxp_handle_t;   //the libfxp handle type (defined internally)

typedef enum {
  FXP_FILE_TYPE_UNKNOWN    = 0,
  FXP_FILE_TYPE_REGULAR    = 1,
  FXP_FILE_TYPE_DIRECTORY,
  FXP_FILE_TYPE_LINK,
  FXP_FILE_TYPE_SOCKET,
} fxp_file_type_t;

typedef struct {
  char name[BUFFSZ];
  fxp_file_type_t type;
  int is_complete;           //if 1, instance of fxp_complete_file_t
} fxp_file_t;                //base file structure

typedef struct {
  char name[BUFFSZ];
  fxp_file_type_t type;
  int is_complete;           //always 1

  unsigned short access;     //file permissions
  int num_hard_links;        //>shrug< why not?
  char owning_user[SMBUFFSZ];
  char owning_group[SMBUFFSZ];
  unsigned long long file_size;
  time_t timestamp;

  char raw_line[BUFFSZ];     //unparsed line from server
} fxp_complete_file_t;       //fxp_file_t + extra file info

typedef fxp_error_t (*fxp_send_hook)(fxp_handle_t fxp, const char *message, unsigned message_len);
typedef fxp_error_t (*fxp_receive_hook)(fxp_handle_t fxp, const char *message, unsigned message_len);

//returns a possibly descriptive string describing given error
const char *fxp_strerror(fxp_handle_t fxp, fxp_error_t error);

//entry point into the library - validates url and sets up handle (call before anything else)
fxp_error_t fxp_init(fxp_handle_t *pfxp, const url_t *url);
//exit point from the library - destroys/invalidates handle and performs final cleanup
fxp_error_t fxp_destroy(fxp_handle_t *pfxp);

//installs the send hook for the control connection - pass NULL to uninstall hook
fxp_error_t fxp_set_control_send_hook(fxp_handle_t fxp, fxp_send_hook hook);
//installs the receive hook for the control connection - pass NULL to uninstall hook
fxp_error_t fxp_set_control_receive_hook(fxp_handle_t fxp, fxp_receive_hook hook);
//sets the control connection receive timeout (in milliseconds, 0 means wait indefinitely)
fxp_error_t fxp_set_control_receive_timeout(fxp_handle_t fxp, unsigned long timeout);
//sets the data connection establishment timeout (in milliseconds, 0 means wait indefinitely)
fxp_error_t fxp_set_data_establish_timeout(fxp_handle_t fxp, unsigned long timeout);
//sets the data connection receive timeout (in milliseconds, 0 means wait indefinitely)
fxp_error_t fxp_set_data_receive_timeout(fxp_handle_t fxp, unsigned long timeout);
//sets the data connection send timeout (in milliseconds, 0 means wait indefinitely)
fxp_error_t fxp_set_data_send_timeout(fxp_handle_t fxp, unsigned long timeout);

//connects to host with an optional timeout (in milliseconds, 0 means wait indefinitely)
fxp_error_t fxp_connect(fxp_handle_t fxp, unsigned long timeout);
//disconnects from host, handle can be used in another call to fxp_connect()
fxp_error_t fxp_disconnect(fxp_handle_t fxp);
//logs into the ftp server (optional username and password, pass NULL to use parsed/default values)
fxp_error_t fxp_login(fxp_handle_t fxp, const char *username, const char *password);
//sets the data transfer type (NOTE: list operations set type to ASCII implicitly)
fxp_error_t fxp_type(fxp_handle_t fxp, fxp_type_t type);
//sets the data connection mode
fxp_error_t fxp_mode(fxp_handle_t fxp, fxp_mode_t mode);
//returns the current working directory
fxp_error_t fxp_pwd(fxp_handle_t fxp, char *out_dirname, unsigned out_dirname_size);
//changes the current working directory
fxp_error_t fxp_cwd(fxp_handle_t fxp, const char *new_dirname);
//ascends a single directory
fxp_error_t fxp_cdup(fxp_handle_t fxp);
//renames a file
fxp_error_t fxp_rename(fxp_handle_t fxp, const char *src_filename, const char *dst_filename);
//deletes a file
fxp_error_t fxp_dele(fxp_handle_t fxp, const char *filename);
//creates a new directory
fxp_error_t fxp_mkd(fxp_handle_t fxp, const char *dirname);
//removes an existing directory
fxp_error_t fxp_rmd(fxp_handle_t fxp, const char *dirname);
//lists files in the specified directory (or current directory if dirname == NULL)
fxp_error_t fxp_nlst(fxp_handle_t fxp, const char *dirname, list_t *out_files);
//lists files in the specified directory (or current directory if dirname == NULL)
fxp_error_t fxp_list(fxp_handle_t fxp, const char *dirname, list_t *out_complete_files);
//returns (possibly complete) file info on specified file (wildcards NOT supported)
fxp_error_t fxp_stat(fxp_handle_t fxp, const char *filename, fxp_complete_file_t *out_file);
//sends NOOP
fxp_error_t fxp_noop(fxp_handle_t fxp);

//opens a remote file for reading
fxp_error_t fxp_retr(fxp_handle_t fxp, const char *filename);
//opens a remote file for reading, starting from given byte offset
fxp_error_t fxp_retr_rest(fxp_handle_t fxp, const char *filename, unsigned long offset);
//returns remote file size ONLY after a successful fxp_retr() or fxp_retr_rest() (see API doc)
fxp_error_t fxp_retr_get_size(fxp_handle_t fxp, unsigned long long *out_size);
//reads from the remote file (closes automatically when done)
fxp_error_t fxp_retr_read(fxp_handle_t fxp, char *out_buff, unsigned *out_buff_size);
//closes the remote file open for reading (redundant call)
fxp_error_t fxp_retr_close(fxp_handle_t fxp);

//opens a remote file for writing
fxp_error_t fxp_stor(fxp_handle_t fxp, const char *filename);
//opens an existing remote file for writing, starting from the given byte offset
fxp_error_t fxp_stor_rest(fxp_handle_t fxp, const char *filename, unsigned long offset);
//writes to the remote file
fxp_error_t fxp_stor_write(fxp_handle_t fxp, const char *buff, unsigned buff_size);
//closes the remote file open for writing
fxp_error_t fxp_stor_close(fxp_handle_t fxp);

//backward-compatibility
typedef fxp_file_t fxp_simple_file_t;
#define fxp_mkdir fxp_mkd
#define fxp_rmdir fxp_rmd

#ifdef __cplusplus
}
#endif


#endif
