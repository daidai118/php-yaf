/*
  +----------------------------------------------------------------------+
  | Yet Another Framework                                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Xinchen Hui  <laruence@php.net>                              |
  +----------------------------------------------------------------------+
 */

/* $Id: static.c 329197 2013-01-18 05:55:37Z laruence $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#include "php_yaf.h"
#include "yaf_namespace.h"
#include "yaf_exception.h"
#include "yaf_request.h"

#include "yaf_router.h"
#include "routes/interface.h"
#include "routes/static.h"

zend_class_entry * yaf_route_static_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(yaf_route_static_match_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, uri)
ZEND_END_ARG_INFO()
/* }}} */

int yaf_route_pathinfo_route(yaf_request_t *request, char *req_uri, int req_uri_len TSRMLS_DC) /* {{{ */ {
	zval *params;
	char *module = NULL, *controller = NULL, *action = NULL, *rest = NULL;

	do {
#define strip_slashs(p) while (*p == ' ' || *p == '/') { ++p; }
		char *s, *p;
		char *uri;

		if (req_uri_len == 0
				|| (req_uri_len == 1 && *req_uri == '/')) {
			break;
		}

		uri = req_uri;
		s = p = uri;

		if (req_uri_len) {
			char *q = req_uri + req_uri_len - 1;
			while (q > req_uri && (*q == ' ' || *q == '/')) {
				*q-- = '\0';
			}
		}

		strip_slashs(p);

		if ((s = strstr(p, "/")) != NULL) {
			if (yaf_application_is_module_name(p, s-p TSRMLS_CC)) {
				module = estrndup(p, s - p);
				p  = s + 1;
		        strip_slashs(p);
				if ((s = strstr(p, "/")) != NULL) {
					controller = estrndup(p, s - p);
					p  = s + 1;
				}
			} else {
				controller = estrndup(p, s - p);
				p  = s + 1;
			}
		}

		strip_slashs(p);
		if ((s = strstr(p, "/")) != NULL) {
			action = estrndup(p, s - p);
			p  = s + 1;
		}

		strip_slashs(p);
		if (*p != '\0') {
			do {
				if (!module && !controller && !action) {
					if (yaf_application_is_module_name(p, strlen(p) TSRMLS_CC)) {
						module = estrdup(p);
						break;
					}
				}

				if (!controller) {
					controller = estrdup(p);
					break;
				}

				if (!action) {
					action = estrdup(p);
					break;
				}

				rest = estrdup(p);
			} while (0);
		}

		if (module && controller == NULL) {
			controller = module;
			module = NULL;
		} else if (module && action == NULL) {
			action = controller;
			controller = module;
			module = NULL;
	    } else if (controller && action == NULL ) {
			/* /controller */
			if (YAF_G(action_prefer)) {
				action = controller;
				controller = NULL;
			}
		}
	} while (0);

	if (module != NULL) {
		zend_update_property_string(yaf_request_ce, request, ZEND_STRL(YAF_REQUEST_PROPERTY_NAME_MODULE), module TSRMLS_CC);
		efree(module);
	}
	if (controller != NULL) {
		zend_update_property_string(yaf_request_ce, request, ZEND_STRL(YAF_REQUEST_PROPERTY_NAME_CONTROLLER), controller TSRMLS_CC);
		efree(controller);
	}

	if (action != NULL) {
		zend_update_property_string(yaf_request_ce, request, ZEND_STRL(YAF_REQUEST_PROPERTY_NAME_ACTION), action TSRMLS_CC);
		efree(action);
	}

	if (rest) {
		params = yaf_router_parse_parameters(rest TSRMLS_CC);
		(void)yaf_request_set_params_multi(request, params TSRMLS_CC);
		zval_ptr_dtor(&params);
		efree(rest);
	}

	return 1;
}
/* }}} */

/** {{{ int yaf_route_static_route(yaf_route_t *route, yaf_request_t *request TSRMLS_DC)
 */
int yaf_route_static_route(yaf_route_t *route, yaf_request_t *request TSRMLS_DC) {
	zval *zuri, *base_uri;
	char *req_uri;
	int  req_uri_len;

	zuri 	 = zend_read_property(yaf_request_ce, request, ZEND_STRL(YAF_REQUEST_PROPERTY_NAME_URI), 1 TSRMLS_CC);
	base_uri = zend_read_property(yaf_request_ce, request, ZEND_STRL(YAF_REQUEST_PROPERTY_NAME_BASE), 1 TSRMLS_CC);

	if (base_uri && IS_STRING == Z_TYPE_P(base_uri)
			&& !strncasecmp(Z_STRVAL_P(zuri), Z_STRVAL_P(base_uri), Z_STRLEN_P(base_uri))) {
		req_uri  = estrdup(Z_STRVAL_P(zuri) + Z_STRLEN_P(base_uri));
		req_uri_len = Z_STRLEN_P(zuri) - Z_STRLEN_P(base_uri);
	} else {
		req_uri  = estrdup(Z_STRVAL_P(zuri));
		req_uri_len = Z_STRLEN_P(zuri);
	}

	yaf_route_pathinfo_route(request, req_uri, req_uri_len TSRMLS_CC);
	efree(req_uri);
	return 1;
}
/* }}} */

/** {{{ proto public Yaf_Router_Static::route(Yaf_Request $req)
*/
PHP_METHOD(yaf_route_static, route) {
	yaf_request_t *request;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &request, yaf_request_ce) == FAILURE) {
		return;
	} else {
		RETURN_BOOL(yaf_route_static_route(getThis(), request TSRMLS_CC));
	}
}
/* }}} */

/** {{{ proto public Yaf_Router_Static::match(string $uri)
*/
PHP_METHOD(yaf_route_static, match) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ yaf_route_static_methods
 */
zend_function_entry yaf_route_static_methods[] = {
	PHP_ME(yaf_route_static, match, yaf_route_static_match_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_route_static, route, yaf_route_route_arginfo, 		ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
 */
YAF_STARTUP_FUNCTION(route_static) {
	zend_class_entry ce;

	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Route_Static", "Yaf\\Route_Static", yaf_route_static_methods);
	yaf_route_static_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	zend_class_implements(yaf_route_static_ce TSRMLS_CC, 1, yaf_route_ce);

	return SUCCESS;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

