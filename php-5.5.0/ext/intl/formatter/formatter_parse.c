/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Stanislav Malyshev <stas@zend.com>                          |
   +----------------------------------------------------------------------+
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unicode/ustring.h>
#include <locale.h>

#include "php_intl.h"
#include "formatter_class.h"
#include "formatter_format.h"
#include "formatter_parse.h"
#include "intl_convert.h"

#define ICU_LOCALE_BUG 1

/* {{{ proto mixed NumberFormatter::parse( string $str[, int $type, int &$position ])
 * Parse a number. }}} */
/* {{{ proto mixed numfmt_parse( NumberFormatter $nf, string $str[, int $type, int &$position ])
 * Parse a number.
 */
PHP_FUNCTION( numfmt_parse )
{
	long type = FORMAT_TYPE_DOUBLE;
	UChar* sstr = NULL;
	int sstr_len = 0;
	char* str = NULL;
	int str_len;
	int32_t val32, position = 0;
	int64_t val64;
	double val_double;
	int32_t* position_p = NULL;
	zval *zposition = NULL;
	char *oldlocale;
	FORMATTER_METHOD_INIT_VARS;

	/* Parse parameters. */
	if( zend_parse_method_parameters( ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|lz!",
		&object, NumberFormatter_ce_ptr,  &str, &str_len, &type, &zposition ) == FAILURE )
	{
		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR,
			"number_parse: unable to parse input params", 0 TSRMLS_CC );

		RETURN_FALSE;
	}

	/* Fetch the object. */
	FORMATTER_METHOD_FETCH_OBJECT;

	/* Convert given string to UTF-16. */
	intl_convert_utf8_to_utf16(&sstr, &sstr_len, str, str_len, &INTL_DATA_ERROR_CODE(nfo));
	INTL_METHOD_CHECK_STATUS( nfo, "String conversion to UTF-16 failed" );

	if(zposition) {
		convert_to_long(zposition);
		position = (int32_t)Z_LVAL_P( zposition );
		position_p = &position;
	}

#if ICU_LOCALE_BUG && defined(LC_NUMERIC)
	oldlocale = setlocale(LC_NUMERIC, "C");
#endif

	switch(type) {
		case FORMAT_TYPE_INT32:
			val32 = unum_parse(FORMATTER_OBJECT(nfo), sstr, sstr_len, position_p, &INTL_DATA_ERROR_CODE(nfo));
			RETVAL_LONG(val32);
			break;
		case FORMAT_TYPE_INT64:
			val64 = unum_parseInt64(FORMATTER_OBJECT(nfo), sstr, sstr_len, position_p, &INTL_DATA_ERROR_CODE(nfo));
			if(val64 > LONG_MAX || val64 < LONG_MIN) {
				RETVAL_DOUBLE(val64);
			} else {
				RETVAL_LONG((long)val64);
			}
			break;
		case FORMAT_TYPE_DOUBLE:
			val_double = unum_parseDouble(FORMATTER_OBJECT(nfo), sstr, sstr_len, position_p, &INTL_DATA_ERROR_CODE(nfo));
			RETVAL_DOUBLE(val_double);
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unsupported format type %ld", type);
			RETVAL_FALSE;
			break;
	}
#if ICU_LOCALE_BUG && defined(LC_NUMERIC)
	setlocale(LC_NUMERIC, oldlocale);
#endif
	if(zposition) {
		zval_dtor(zposition);
		ZVAL_LONG(zposition, position);
	}

	if (sstr) {
		efree(sstr);
	}

	INTL_METHOD_CHECK_STATUS( nfo, "Number parsing failed" );
}
/* }}} */

/* {{{ proto double NumberFormatter::parseCurrency( string $str, string $&currency[, int $&position] )
 * Parse a number as currency. }}} */
/* {{{ proto double numfmt_parse_currency( NumberFormatter $nf, string $str, string $&currency[, int $&position] )
 * Parse a number as currency.
 */
PHP_FUNCTION( numfmt_parse_currency )
{
	double number;
	UChar currency[5] = {0};
	UChar* sstr = NULL;
	int sstr_len = 0;
	char *currency_str = NULL;
	int currency_len = 0;
	char *str;
	int str_len;
	int32_t* position_p = NULL;
	int32_t position = 0;
	zval *zcurrency, *zposition = NULL;
	FORMATTER_METHOD_INIT_VARS;

	/* Parse parameters. */
	if( zend_parse_method_parameters( ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz|z!",
		&object, NumberFormatter_ce_ptr,  &str, &str_len, &zcurrency, &zposition ) == FAILURE )
	{
		intl_error_set( NULL, U_ILLEGAL_ARGUMENT_ERROR,
			"number_parse_currency: unable to parse input params", 0 TSRMLS_CC );

		RETURN_FALSE;
	}

	/* Fetch the object. */
	FORMATTER_METHOD_FETCH_OBJECT;

	/* Convert given string to UTF-16. */
	intl_convert_utf8_to_utf16(&sstr, &sstr_len, str, str_len, &INTL_DATA_ERROR_CODE(nfo));
	INTL_METHOD_CHECK_STATUS( nfo, "String conversion to UTF-16 failed" );

	if(zposition) {
		convert_to_long(zposition);
		position = (int32_t)Z_LVAL_P( zposition );
		position_p = &position;
	}

	number = unum_parseDoubleCurrency(FORMATTER_OBJECT(nfo), sstr, sstr_len, position_p, currency, &INTL_DATA_ERROR_CODE(nfo));
	if(zposition) {
		zval_dtor(zposition);
		ZVAL_LONG(zposition, position);
	}
	if (sstr) {
		efree(sstr);
	}
	INTL_METHOD_CHECK_STATUS( nfo, "Number parsing failed" );

	/* Convert parsed currency to UTF-8 and pass it back to caller. */
	intl_convert_utf16_to_utf8(&currency_str, &currency_len, currency, u_strlen(currency), &INTL_DATA_ERROR_CODE(nfo));
	INTL_METHOD_CHECK_STATUS( nfo, "Currency conversion to UTF-8 failed" );
	zval_dtor( zcurrency );
	ZVAL_STRINGL(zcurrency, currency_str, currency_len, 0);

	RETVAL_DOUBLE( number );
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
