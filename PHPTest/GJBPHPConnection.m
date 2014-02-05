//
//  GJBPHPConnection.m
//  PHPTest
//
//  Created by Grant Butler on 7/4/13.
//  Copyright (c) 2013 Grant Butler. All rights reserved.
//

#import "GJBPHPConnection.h"
#import "HTTPMessage.h"
#import "GJBPHPResponse.h"
#import "HTTPErrorResponse.h"

#include "php.h"
#include "SAPI.h"
#include "php_main.h"
#include "php_variables.h"
#include "TSRM.h"

@interface GJBPHPConnection ()

@property (nonatomic) GJBPHPResponse *response;
@property (nonatomic) UInt64 requestBodyOffset;

@property (nonatomic, readonly) HTTPMessage *request;
@property (nonatomic, readonly) HTTPConfig *config;

@end

static int php_objc_startup(sapi_module_struct *sapi_module) {
	if (php_module_startup(sapi_module, NULL, 0) == FAILURE) {
		return FAILURE;
	}
	return SUCCESS;
}

static int php_objc_ub_write(const char *str, unsigned int str_length TSRMLS_DC) {
	GJBPHPConnection *connection = (__bridge GJBPHPConnection *)SG(server_context);
	
	[connection.response.responseData appendBytes:str length:str_length];
	
	return SUCCESS;
}

static int php_objc_header_handler(sapi_header_struct *sapi_header, sapi_header_op_enum op, sapi_headers_struct *sapi_headers TSRMLS_DC) {
	GJBPHPConnection *connection = (__bridge GJBPHPConnection *)SG(server_context);
	
	if (op == SAPI_HEADER_DELETE_ALL) {
		[connection.response.httpHeaders removeAllObjects];
		
		return 0;
	}
	else if (op == SAPI_HEADER_DELETE) {
		NSString *header = [NSString stringWithUTF8String:sapi_header->header];
		NSArray *headerComponents = [header componentsSeparatedByString:@":"];
		NSString *headerName = [headerComponents[0] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
		[connection.response.httpHeaders removeObjectForKey:headerName];
		
		return 0;
	}
	else if (op == SAPI_HEADER_ADD || op == SAPI_HEADER_REPLACE) {
		NSString *header = [NSString stringWithUTF8String:sapi_header->header];
		NSArray *headerComponents = [header componentsSeparatedByString:@":"];
		NSString *headerName = [headerComponents[0] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
		NSString *headerValue = [headerComponents[1] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
		
		if (op == SAPI_HEADER_REPLACE || [headerName isEqualToString:@"Content-Type"]) {
			[connection.response.httpHeaders removeObjectForKey:headerName];
		}
		
		[connection.response.httpHeaders setObject:headerValue forKey:headerName];
		
		return SAPI_HEADER_ADD;
	}
	
	return 0;
}

static int php_objc_send_headers(sapi_headers_struct *sapi_headers TSRMLS_DC) {
	return SAPI_HEADER_SENT_SUCCESSFULLY;
}

static int php_objc_read_post(char *buffer, uint count_bytes TSRMLS_DC) {
	GJBPHPConnection *connection = (__bridge GJBPHPConnection *)SG(server_context);
	
	count_bytes = MIN(count_bytes, [[connection.request body] length] - connection.requestBodyOffset);
	
	[[connection.request body] getBytes:buffer range:NSMakeRange(connection.requestBodyOffset, count_bytes)];
	
	connection.requestBodyOffset += count_bytes;
	
	return count_bytes;
}

static char *php_objc_read_cookies(TSRMLS_D) {
	GJBPHPConnection *connection = (__bridge GJBPHPConnection *)SG(server_context);
	
	return (char *)[[connection.request headerField:@"Cookie"] UTF8String];
}

static void php_objc_register_variable(zval *track_vars_array, const char *key, const char *val TSRMLS_DC) /* {{{ */
{
	char *new_val = (char *)val;
	uint new_val_len;
	if (sapi_module.input_filter(PARSE_SERVER, (char*)key, &new_val, strlen(val), &new_val_len TSRMLS_CC)) {
		php_register_variable_safe((char *)key, new_val, new_val_len, track_vars_array TSRMLS_CC);
	}
} /* }}} */

static void php_objc_register_server_variables(zval *track_vars_array TSRMLS_DC) {
	GJBPHPConnection *connection = (__bridge GJBPHPConnection *)SG(server_context);
	
	for (NSString *headerName in [connection.request allHeaderFields]) {
		NSString *headerValue = [connection.request allHeaderFields][headerName];
		NSString *serverVariableName = [@"HTTP_" stringByAppendingString:[[headerName uppercaseString] stringByReplacingOccurrencesOfString:@"-" withString:@"_"]];
		
		php_objc_register_variable(track_vars_array, [serverVariableName UTF8String], [headerValue UTF8String]);
	}
	
	php_objc_register_variable(track_vars_array, "SERVER_PROTOCOL", [[connection.request version] UTF8String]);
	
	php_objc_register_variable(track_vars_array, "DOCUMENT_ROOT", [[connection.config documentRoot] UTF8String]);
	
	if (SG(request_info).query_string) {
		php_objc_register_variable(track_vars_array, "QUERY_STRING", SG(request_info).query_string);
	}
	else {
		php_objc_register_variable(track_vars_array, "QUERY_STRING", "");
	}
	
	php_objc_register_variable(track_vars_array, "PHP_SELF", SG(request_info).request_uri);
	php_objc_register_variable(track_vars_array, "REQUEST_URI", SG(request_info).request_uri);
	php_objc_register_variable(track_vars_array, "REQUEST_METHOD", SG(request_info).request_method);
//	php_objc_register_variable(track_vars_array, "PATH_TRANSLATED", SG(request_info).path_translated);
	
}

sapi_module_struct php_objective_c_module = {
	"objc",                       /* name */
	"Objective-C Module",        /* pretty name */
	
	php_objc_startup,              /* startup */
	php_module_shutdown_wrapper,   /* shutdown */
	
	NULL,                          /* activate */
	NULL,           /* deactivate */
	
	php_objc_ub_write,             /* unbuffered write */
	NULL,                /* flush */
	NULL,                          /* get uid */
	NULL,                          /* getenv */
	
	php_error,                     /* error handler */
	
	php_objc_header_handler,                          /* header handler */
	php_objc_send_headers,                          /* send headers handler */
	NULL,          /* send header handler */
	
	php_objc_read_post,                          /* read POST data */
	php_objc_read_cookies,         /* read Cookies */
	
	php_objc_register_server_variables,   /* register server variables */
	NULL,          /* Log message */
	NULL,							/* Get request time */
	NULL,							/* Child terminate */
	
	STANDARD_SAPI_MODULE_PROPERTIES
};

@implementation GJBPHPConnection

+ (void)initialize {
	if (self == [GJBPHPConnection class]) {
		sapi_module_struct *sapi_module = &php_objective_c_module;
		
		sapi_startup(sapi_module);
		
		sapi_module->startup(sapi_module);
	}
}

- (HTTPMessage *)request {
	return self->request;
}

- (HTTPConfig *)config {
	return self->config;
}

- (NSObject<HTTPResponse> *)httpResponseForMethod:(NSString *)method URI:(NSString *)path {
	NSString *filePath = [self filePathForURI:path];
	
	NSLog(@"%@", path);
	
	if ([[filePath pathExtension] isEqualToString:@"php"]) {
		self.response = [[GJBPHPResponse alloc] init];
		self.requestBodyOffset = 0;
		
		SG(server_context) = (void *)CFBridgingRetain(self);
		SG(request_info).query_string = (char *)[[[self->request url] query] UTF8String];
		SG(request_info).request_uri = (char *)[[self requestURI] UTF8String];
		SG(request_info).request_method = (char *)[[self->request method] UTF8String];
//		SG(request_info).path_translated = (char *)[filePath UTF8String];
		
		NSString *requestContentType = [self->request headerField:@"Content-Type"];
		
		if (requestContentType) {
			SG(request_info).content_type = (char *)[requestContentType UTF8String];
		}
		else {
			SG(request_info).content_type = NULL;
		}
		
		SG(request_info).content_length = [[self->request body] length];
		
		zend_file_handle file_handle = {0};
		file_handle.type = ZEND_HANDLE_FILENAME;
		file_handle.filename = [filePath UTF8String];
		file_handle.free_filename = 0;
		file_handle.opened_path = NULL;
		
		if (php_request_startup(TSRMLS_C) == SUCCESS) {
			php_execute_script(&file_handle TSRMLS_CC);
			php_request_shutdown(NULL);
			
			CFBridgingRelease(SG(server_context));
			
			return self.response;
		}
		else {
			// TODO: Subclass HTTPErrorResponse to actually return web pages with error messages.
			return [[HTTPErrorResponse alloc] initWithErrorCode:500];
		}
	}
	
	return [super httpResponseForMethod:method URI:path];
}

@end
