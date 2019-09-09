#ifndef _RCOM_STREAMERLINK_H_
#define _RCOM_STREAMERLINK_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _streamerlink_t streamerlink_t;
typedef struct _response_t response_t;

typedef int (*streamerlink_ondata_t)(void *userdata, const char *buf, int len);
typedef int (*streamerlink_onresponse_t)(void *userdata, response_t *response);

addr_t *streamerlink_addr(streamerlink_t *link);
int streamerlink_connect(streamerlink_t *link);
int streamerlink_disconnect(streamerlink_t *link);

#ifdef __cplusplus
}
#endif

#endif // _RCOM_STREAMERLINK_H_
