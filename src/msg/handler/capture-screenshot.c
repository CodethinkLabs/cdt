
#include <stddef.h>
#include <stdbool.h>

#include "msg/msg.h"
#include "msg/queue.h"
#include "msg/private.h"

#define PRINT_FMT_CAPTURE_SCREENSHOT__FMT \
	"{" \
		"\"id\":%i," \
		"\"method\":\"Page.captureScreenshot\"," \
		"\"params\":{" \
			"\"format\":\"%s\"" \
		"}" \
	"}"

char *msg_str_capture_screenshot(const struct msg *msg, int id)
{
	struct msg_container *m;
	const char *fmt = msg->data.capture_screenshot.format;

	if (fmt == NULL) {
		fmt = "png";
	}

	if (!msg_create(&m, PRINT_FMT_CAPTURE_SCREENSHOT__FMT, id, fmt)) {
		return NULL;
	}

	m->type = msg->type;
	m->id = id;

	return m->str;
}
