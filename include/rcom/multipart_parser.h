#ifndef _RCOM_MULTIPART_PARSER_H_
#define _RCOM_MULTIPART_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*multipart_onheaders_t)(void *userdata,
                                    int len,
                                    const char *mimetype,
                                    double timestamp,
                                    uint32_t offset);

typedef int (*multipart_onpart_t)(void *userdata,
                                  const unsigned char *data, int len,
                                  const char *mimetype,
                                  double timestamp);

typedef struct _multipart_parser_t multipart_parser_t;

multipart_parser_t *new_multipart_parser(void* userdata,
                                         multipart_onheaders_t onheaders,
                                         multipart_onpart_t onpart);
void delete_multipart_parser(multipart_parser_t *m);

// Parse a buffer of data
int multipart_parser_process(multipart_parser_t *m, response_t *response,
                             const char *data, int len);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_MULTIPART_PARSER_H_
