/*	
    WebSubresourceClient.m
    Copyright (c) 2002, Apple Computer, Inc. All rights reserved.
*/

#import <WebKit/WebSubresourceClient.h>

#import <WebKit/WebAssertions.h>
#import <WebKit/WebBridge.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebFrame.h>
#import <WebKit/WebKitErrorsPrivate.h>
#import <WebKit/WebViewPrivate.h>

#import <Foundation/NSError_NSURLExtras.h>
#import <Foundation/NSURLRequestPrivate.h>
#import <Foundation/NSURLResponse.h>

#import <WebCore/WebCoreResourceLoader.h>

@implementation WebSubresourceClient

- initWithLoader:(id <WebCoreResourceLoader>)l dataSource:(WebDataSource *)s
{
    [super init];
    
    loader = [l retain];

    [self setDataSource: s];
    
    return self;
}

- (void)dealloc
{
    [loader release];
    [super dealloc];
}

+ (WebSubresourceClient *)startLoadingResource:(id <WebCoreResourceLoader>)rLoader
				   withRequest:(NSMutableURLRequest *)newRequest
                                 customHeaders:(NSDictionary *)customHeaders
				      referrer:(NSString *)referrer 
				 forDataSource:(WebDataSource *)source
{
    WebSubresourceClient *client = [[[self alloc] initWithLoader:rLoader dataSource:source] autorelease];
    
    [source _addSubresourceClient:client];


    NSEnumerator *e = [customHeaders keyEnumerator];
    NSString *key;
    while ((key = (NSString *)[e nextObject]) != nil) {
	[newRequest addValue:[customHeaders objectForKey:key] forHTTPHeaderField:key];
    }

    // Use the original request's cache policy for two reasons:
    // 1. For POST requests, we mutate the cache policy for the main resource,
    //    but we do not want this to apply to subresources
    // 2. Delegates that modify the cache policy using willSendRequest: should
    //    not affect any other resources. Such changes need to be done
    //    per request.
    [newRequest setCachePolicy:[[source _originalRequest] cachePolicy]];
    [newRequest setHTTPReferrer:referrer];
    
    WebView *_webView = [source _webView];
    [newRequest setMainDocumentURL:[[[[_webView mainFrame] dataSource] request] URL]];
    [newRequest setHTTPUserAgent:[_webView userAgentForURL:[newRequest URL]]];
    
    BOOL succeeded = [client loadWithRequest:newRequest];
        
    if (!succeeded) {
        [source _removeSubresourceClient:client];

        [rLoader reportError];

        NSError *badURLError = [[NSError alloc] _webKitErrorWithDomain:NSURLErrorDomain 
                                                                  code:NSURLErrorBadURL
                                                                   URL:[newRequest URL]];
        [_webView _receivedError:badURLError fromDataSource:source];
        [badURLError release];
        client = nil;
    }
    
    return client;
}

+ (WebSubresourceClient *)startLoadingResource:(id <WebCoreResourceLoader>)rLoader
				       withURL:(NSURL *)URL
				 customHeaders:(NSDictionary *)customHeaders
				      referrer:(NSString *)referrer
				 forDataSource:(WebDataSource *)source
{
    NSMutableURLRequest *newRequest = [[NSMutableURLRequest alloc] initWithURL:URL];
    WebSubresourceClient *client = [self startLoadingResource:rLoader withRequest:newRequest customHeaders:customHeaders referrer:referrer forDataSource:source];
    [newRequest release];

    return client;
}

+ (WebSubresourceClient *)startLoadingResource:(id <WebCoreResourceLoader>)rLoader
				       withURL:(NSURL *)URL
				 customHeaders:(NSDictionary *)customHeaders
				      postData:(NSArray *)postData
				      referrer:(NSString *)referrer
				 forDataSource:(WebDataSource *)source
{
    NSMutableURLRequest *newRequest = [[NSMutableURLRequest alloc] initWithURL:URL];

    [newRequest setHTTPMethod:@"POST"];

    // FIXME: This will have to be expanded to handle filenames and arrays with more than one element to fix file uploading.
    if ([postData count] == 1 && [[postData objectAtIndex:0] isKindOfClass:[NSData class]]) {
        [newRequest setHTTPBody:(NSData *)[postData objectAtIndex:0]];
    }

    WebSubresourceClient *client = [self startLoadingResource:rLoader withRequest:newRequest customHeaders:customHeaders referrer:referrer forDataSource:source];
    [newRequest release];

    return client;

}

- (void)receivedError:(NSError *)error
{
    [[dataSource _webView] _receivedError:error fromDataSource:dataSource];
}

- (NSURLRequest *)willSendRequest:(NSURLRequest *)newRequest redirectResponse:(NSURLResponse *)redirectResponse;
{
    NSURL *oldURL = [request URL];
    NSURLRequest *clientRequest = [super willSendRequest:newRequest redirectResponse:redirectResponse];
    
    if (![oldURL isEqual:[clientRequest URL]]) {
	[loader redirectedToURL:[clientRequest URL]];
    }

    return clientRequest;
}

- (void)didReceiveResponse:(NSURLResponse *)r
{
    ASSERT(r);
    // retain/release self in this delegate method since the additional processing can do
    // anything including possibly releasing self; one example of this is 3266216
    [self retain];
    [loader receivedResponse:r];
    [super didReceiveResponse:r];
    [self release];
}

- (void)didReceiveData:(NSData *)data lengthReceived:(long long)lengthReceived
{
    // retain/release self in this delegate method since the additional processing can do
    // anything including possibly releasing self; one example of this is 3266216
    [self retain];
    [loader addData:data];
    [super didReceiveData:data lengthReceived:lengthReceived];
    [self release];
}

- (void)didFinishLoading
{
    // Calling _removeSubresourceClient will likely result in a call to release, so we must retain.
    [self retain];
    
    [loader finish];
    
    [dataSource _removeSubresourceClient:self];
    
    [[dataSource _webView] _finishedLoadingResourceFromDataSource:dataSource];

    [super didFinishLoading];

    [self release];    
}

- (void)didFailWithError:(NSError *)error
{
    // Calling _removeSubresourceClient will likely result in a call to release, so we must retain.
    [self retain];
    
    [loader reportError];
    [dataSource _removeSubresourceClient:self];
    [self receivedError:error];
    [super didFailWithError:error];

    [self release];
}

- (void)cancel
{
    // Calling _removeSubresourceClient will likely result in a call to release, so we must retain.
    [self retain];
        
    [loader cancel];
    [dataSource _removeSubresourceClient:self];
    [self receivedError:[self cancelledError]];
    [super cancel];

    [self release];
}

@end
