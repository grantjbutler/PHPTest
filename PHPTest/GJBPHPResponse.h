//
//  GJBPHPResponse.h
//  PHPTest
//
//  Created by Grant Butler on 7/4/13.
//  Copyright (c) 2013 Grant Butler. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "HTTPResponse.h"

@interface GJBPHPResponse : NSObject <HTTPResponse>

@property (nonatomic, readonly) NSMutableData *responseData;
@property (nonatomic, readonly) NSMutableDictionary *httpHeaders;
@property (nonatomic) BOOL isDone;

//- (NSInteger)status;

@end
