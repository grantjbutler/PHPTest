//
//  GJBPHPResponse.m
//  PHPTest
//
//  Created by Grant Butler on 7/4/13.
//  Copyright (c) 2013 Grant Butler. All rights reserved.
//

#import "GJBPHPResponse.h"

@implementation GJBPHPResponse {
	UInt64 _offset;
}

- (id)init {
	self = [super init];
	if (self) {
		_responseData = [[NSMutableData alloc] init];
		_httpHeaders = [[NSMutableDictionary alloc] init];
		_offset = 0;
	}
	return self;
}

- (UInt64)contentLength {
	return [self.responseData length];
}

- (void)setOffset:(UInt64)offset {
	_offset = offset;
}

- (UInt64)offset {
	return _offset;
}

- (NSData *)readDataOfLength:(NSUInteger)length {
	NSUInteger remaining = [self.responseData length] - _offset;
	length = MIN(length, remaining);
	
	void *bytes = (void *)([self.responseData bytes] + _offset);
	
	_offset += length;
	
	return [NSData dataWithBytesNoCopy:bytes length:length freeWhenDone:NO];
}

@end
