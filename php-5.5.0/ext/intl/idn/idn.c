/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 2009 The PHP Group                                     |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Pierre A. Joye <pierre@php.net>                              |
   |         Gustavo Lopes  <cataphract@php.net>                          |
   +----------------------------------------------------------------------+
 */
/* $Id$ */

/* {{{ includes */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>

#include <unicode/uidna.h>
#include <unicode/ustring.h>
#include "ext/standard/php_string.h"

#include "intl_error.h"
#include "intl_convert.h"
/* }}} */

#ifdef UIDNA_INFO_INITIALIZER
#define HAVE_46_API 1 /* has UTS#46 API (introduced in ICU 4.6) */
#endif

enum {
	INTL_IDN_VARIANT_2003 = 0,
	INTL_IDN_VARIANT_UTS46
};

/* {{{ grapheme_register_constants
 * Register API constants
 */
void idn_register_constants( INIT_FUNC_ARGS )
{
	/* OPTIONS */

	/* Option to prohibit processing of unassigned codepoints in the input and
	   do not check if the input conforms to STD-3 ASCII rules. */
	REGISTER_LONG_CONSTANT("IDNA_DEFAULT", UIDNA_DEFAULT, CONST_CS | CONST_PERSISTENT);

	/* Option to allow processing of unassigned codepoints in the input */
	REGISTER_LONG_CONSTANT("IDNA_ALLOW_UNASSIGNED", UIDNA_ALLOW_UNASSIGNED, CONST_CS | CONST_PERSISTENT);

	/* Option to check if input conforms to STD-3 ASCII rules */
	REGISTER_LONG_CONSTANT("IDNA_USE_STD3_RULES", UIDNA_USE_STD3_RULES, CONST_CS | CONST_PERSISTENT);

#ifdef HAVE_46_API

	/* Option to check for whether the input conforms to the BiDi rules.
	 * Ignored by the IDNA2003 implementation. (IDNA2003 always performs a BiDi check.) */
	REGISTER_LONG_CONSTANT("IDNA_CHECK_BIDI", UIDNA_CHECK_BIDI, CONST_CS | CONST_PERSISTENT);

	/* Option to check for whether the input conforms to the CONTEXTJ rules.
	 * Ignored by the IDNA2003 implementation. (The CONTEXTJ check is new in IDNA2008.) */
	REGISTER_LONG_CONSTANT("IDNA_CHECK_CONTEXTJ", UIDNA_CHECK_CONTEXTJ, CONST_CS | CONST_PERSISTENT);

	/* Option for nontransitional processing in ToASCII().
	 * By default, ToASCII() uses transitional processing.
	 * Ignored by the IDNA2003 implementation. */
	REGISTER_LONG_CONSTANT("IDNA_NONTRANSITIONAL_TO_ASCII", UIDNA_NONTRANSITIONAL_TO_ASCII, CONST_CS | CONST_PERSISTENT);

	/* Option for nontransitional processing in ToUnicode().
	 * By default, ToUnicode() uses transitional processing.
	 * Ignored by the IDNA2003 implementation. */
	REGISTER_LONG_CONSTANT("IDNA_NONTRANSITIONAL_TO_UNICODE", UIDNA_NONTRANSITIONAL_TO_UNICODE, CONST_CS | CONST_PERSISTENT);
#endif

	/* VARIANTS */
	REGISTER_LONG_CONSTANT("INTL_IDNA_VARIANT_2003", INTL_IDN_VARIANT_2003, CONST_CS | CONST_PERSISTENT);
#ifdef HAVE_46_API
	REGISTER_LONG_CONSTANT("INTL_IDNA_VARIANT_UTS46", INTL_IDN_VARIANT_UTS46, CONST_CS | CONST_PERSISTENT);
#endif

#ifdef HAVE_46_API
	/* PINFO ERROR CODES */
	REGISTER_LONG_CONSTANT("IDNA_ERROR_EMPTY_LABEL", UIDNA_ERROR_EMPTY_LABEL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_LABEL_TOO_LONG", UIDNA_ERROR_LABEL_TOO_LONG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_DOMAIN_NAME_TOO_LONG", UIDNA_ERROR_DOMAIN_NAME_TOO_LONG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_LEADING_HYPHEN", UIDNA_ERROR_LEADING_HYPHEN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_TRAILING_HYPHEN", UIDNA_ERROR_TRAILING_HYPHEN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_HYPHEN_3_4", UIDNA_ERROR_HYPHEN_3_4, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_LEADING_COMBINING_MARK", UIDNA_ERROR_LEADING_COMBINING_MARK, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_DISALLOWED", UIDNA_ERROR_DISALLOWED, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_PUNYCODE", UIDNA_ERROR_PUNYCODE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_LABEL_HAS_DOT", UIDNA_ERROR_LABEL_HAS_DOT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_INVALID_ACE_LABEL", UIDNA_ERROR_INVALID_ACE_LABEL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_BIDI", UIDNA_ERROR_BIDI, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("IDNA_ERROR_CONTEXTJ", UIDNA_ERROR_CONTEXTJ, CONST_CS | CONST_PERSISTENT);
#endif
}
/* }}} */

enum {
	INTL_IDN_TO_ASCII = 0,
	INTL_IDN_TO_UTF8
};

/* like INTL_CHECK_STATUS, but as a function and varying the name of the func */
static int php_intl_idn_check_status(UErrorCode err, const char *msg, int mode TSRMLS_DC)
{
	intl_error_set_code(NULL, err TSRMLS_CC);
	if (U_FAILURE(err)) {
		char *buff;
		spprintf(&buff, 0, "%s: %s",
			mode == INTL_IDN_TO_ASCII ? "idn_to_ascii" : "idn_to_utf8",
			msg);
		intl_error_set_custom_msg(NULL, buff, 1 TSRMLS_CC);
		efree(buff);
		return FAILURE;
	}

	return SUCCESS;
}

static inline void php_intl_bad_args(const char *msg, int mode TSRMLS_DC)
{
	php_intl_idn_check_status(U_ILLEGAL_ARGUMENT_ERROR, msg, mode TSRMLS_CC);
}

#ifdef HAVE_46_API
static void php_intl_idn_to_46(INTERNAL_FUNCTION_PARAMETERS,
		const char *domain, int domain_len, uint32_t option, int mode, zval *idna_info)
{
	UErrorCode	  status = U_ZERO_ERROR;
	UIDNA		  *uts46;
	int32_t		  len;
	int32_t		  buffer_capac = 255; /* no domain name may exceed this */
	char		  *buffer = emalloc(buffer_capac);
	UIDNAInfo	  info = UIDNA_INFO_INITIALIZER;
	int			  buffer_used = 0;
	
	uts46 = uidna_openUTS46(option, &status);
	if (php_intl_idn_check_status(status, "failed to open UIDNA instance",
			mode TSRMLS_CC) == FAILURE) {
		efree(buffer);
		RETURN_FALSE;
	}

	if (mode == INTL_IDN_TO_ASCII) {
		len = uidna_nameToASCII_UTF8(uts46, domain, (int32_t)domain_len,
				buffer, buffer_capac, &info, &status);
	} else {
		len = uidna_nameToUnicodeUTF8(uts46, domain, (int32_t)domain_len,
				buffer, buffer_capac, &info, &status);
	}
	if (php_intl_idn_check_status(status, "failed to convert name",
			mode TSRMLS_CC) == FAILURE) {
		uidna_close(uts46);
		efree(buffer);
		RETURN_FALSE;
	}
	if (len >= 255) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "ICU returned an unexpected length");
	}

	buffer[len] = '\0';

	if (info.errors == 0) {
		RETVAL_STRINGL(buffer, len, 0);
		buffer_used = 1;
	} else {
		RETVAL_FALSE;
	}

	if (idna_info) {
		if (buffer_used) { /* used in return_value then */
			zval_addref_p(return_value);
			add_assoc_zval_ex(idna_info, "result", sizeof("result"), return_value);
		} else {
			zval *zv;
			ALLOC_INIT_ZVAL(zv);
			ZVAL_STRINGL(zv, buffer, len, 0);
			buffer_used = 1;
			add_assoc_zval_ex(idna_info, "result", sizeof("result"), zv);
		}
		add_assoc_bool_ex(idna_info, "isTransitionalDifferent",
				sizeof("isTransitionalDifferent"), info.isTransitionalDifferent);
		add_assoc_long_ex(idna_info, "errors", sizeof("errors"), (long)info.errors);
	}

	if (!buffer_used) {
		efree(buffer);
	}

	uidna_close(uts46);
}
#endif

static void php_intl_idn_to(INTERNAL_FUNCTION_PARAMETERS,
		const char *domain, int domain_len, uint32_t option, int mode)
{
	UChar* ustring = NULL;
	int ustring_len = 0;
	UErrorCode status;
	char     *converted_utf8;
	int32_t   converted_utf8_len;
	UChar     converted[MAXPATHLEN];
	int32_t   converted_ret_len;

	/* convert the string to UTF-16. */
	status = U_ZERO_ERROR;
	intl_convert_utf8_to_utf16(&ustring, &ustring_len, domain, domain_len, &status);

	if (U_FAILURE(status)) {
		intl_error_set_code(NULL, status TSRMLS_CC);

		/* Set error messages. */
		intl_error_set_custom_msg( NULL, "Error converting input string to UTF-16", 0 TSRMLS_CC );
		if (ustring) {
			efree(ustring);
		}
		RETURN_FALSE;
	} else {
		UParseError parse_error;

		status = U_ZERO_ERROR;
		if (mode == INTL_IDN_TO_ASCII) {
			converted_ret_len = uidna_IDNToASCII(ustring, ustring_len, converted, MAXPATHLEN, (int32_t)option, &parse_error, &status);
		} else {
			converted_ret_len = uidna_IDNToUnicode(ustring, ustring_len, converted, MAXPATHLEN, (int32_t)option, &parse_error, &status);
		}
		efree(ustring);

		if (U_FAILURE(status)) {
			intl_error_set( NULL, status, "idn_to_ascii: cannot convert to ASCII", 0 TSRMLS_CC );
			RETURN_FALSE;
		}

		status = U_ZERO_ERROR;
		intl_convert_utf16_to_utf8(&converted_utf8, &converted_utf8_len, converted, converted_ret_len, &status);

		if (U_FAILURE(status)) {
			/* Set global error code. */
			intl_error_set_code(NULL, status TSRMLS_CC);

			/* Set error messages. */
			intl_error_set_custom_msg( NULL, "Error converting output string to UTF-8", 0 TSRMLS_CC );
			efree(converted_utf8);
			RETURN_FALSE;
		}
	}

	/* return the allocated string, not a duplicate */
	RETURN_STRINGL(((char *)converted_utf8), converted_utf8_len, 0);
}

static void php_intl_idn_handoff(INTERNAL_FUNCTION_PARAMETERS, int mode)
{
	char *domain;
	int domain_len;
	long option = 0,
		 variant = INTL_IDN_VARIANT_2003;
	zval *idna_info = NULL;

	intl_error_reset(NULL TSRMLS_CC);

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|llz",
			&domain, &domain_len, &option, &variant, &idna_info) == FAILURE) {
		php_intl_bad_args("bad arguments", mode TSRMLS_CC);
		RETURN_NULL(); /* don't set FALSE because that's not the way it was before... */
	}

#ifdef HAVE_46_API
	if (variant != INTL_IDN_VARIANT_2003 && variant != INTL_IDN_VARIANT_UTS46) {
		php_intl_bad_args("invalid variant, must be one of {"
			"INTL_IDNA_VARIANT_2003, INTL_IDNA_VARIANT_UTS46}", mode TSRMLS_CC);
		RETURN_FALSE;
	}
#else
	if (variant != INTL_IDN_VARIANT_2003) {
		php_intl_bad_args("invalid variant, PHP was compiled against "
			"an old version of ICU and only supports INTL_IDN_VARIANT_2003",
			mode TSRMLS_CC);
		RETURN_FALSE;
	}
#endif

	if (domain_len < 1) {
		php_intl_bad_args("empty domain name", mode TSRMLS_CC);
		RETURN_FALSE;
	}
	if (domain_len > INT32_MAX - 1) {
		php_intl_bad_args("domain name too large", mode TSRMLS_CC);
		RETURN_FALSE;
	}
	/* don't check options; it wasn't checked before */

	if (idna_info != NULL) {
		if (variant == INTL_IDN_VARIANT_2003) {
			php_error_docref0(NULL TSRMLS_CC, E_NOTICE,
				"4 arguments were provided, but INTL_IDNA_VARIANT_2003 only "
				"takes 3 - extra argument ignored");
		} else {
			zval_dtor(idna_info);
			array_init(idna_info);
		}
	}
	
	if (variant == INTL_IDN_VARIANT_2003) {
		php_intl_idn_to(INTERNAL_FUNCTION_PARAM_PASSTHRU,
				domain, domain_len, (uint32_t)option, mode);
	}
#ifdef HAVE_46_API
	else {
		php_intl_idn_to_46(INTERNAL_FUNCTION_PARAM_PASSTHRU, domain, domain_len,
				(uint32_t)option, mode, idna_info);
	}
#endif
}

/* {{{ proto int idn_to_ascii(string domain[, int options[, int variant[, array &idna_info]]])
   Converts an Unicode domain to ASCII representation, as defined in the IDNA RFC */
PHP_FUNCTION(idn_to_ascii)
{
	php_intl_idn_handoff(INTERNAL_FUNCTION_PARAM_PASSTHRU, INTL_IDN_TO_ASCII);
}
/* }}} */


/* {{{ proto int idn_to_utf8(string domain[, int options[, int variant[, array &idna_info]]])
   Converts an ASCII representation of the domain to Unicode (UTF-8), as defined in the IDNA RFC */
PHP_FUNCTION(idn_to_utf8)
{
	php_intl_idn_handoff(INTERNAL_FUNCTION_PARAM_PASSTHRU, INTL_IDN_TO_UTF8);
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
