#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include <string>

#include <allegro5/allegro.h>

#if defined ALLEGRO_IPHONE
#import <UIKit/UIKit.h>
#include <allegro5/allegro_iphone.h>
#include <allegro5/allegro_iphone_objc.h>
#endif

static ALLEGRO_DISPLAY *allegro_display;

// Admob stuff
#ifdef ADMOB
#import <GoogleMobileAds/GoogleMobileAds.h>
#import <GoogleMobileAds/GADInterstitialDelegate.h>

#import <StoreKit/StoreKit.h>

#import "Reachability.h"

#include "apple.h"

static ALLEGRO_DISPLAY *allegro_display;
static volatile int queried_purchased = -1;
static volatile int pay_purchased = -1;
static volatile int purchased = -1;
static volatile bool really_checking_purchase = false;

static GADInterstitial *interstitial;
static int count = 0;

void requestNewInterstitial();

@interface Ad_Delegate : NSObject<GADInterstitialDelegate>
{
}
- (void)interstitialWillDismissScreen:(nonnull GADInterstitial *)ad;
@end

@implementation Ad_Delegate
- (void)interstitialWillDismissScreen:(nonnull GADInterstitial *)ad
{
	requestNewInterstitial();
}
@end

static void *request_thread(void *arg)
{
	al_rest(5.0);
	Ad_Delegate *ad_delegate = [[Ad_Delegate alloc] init];
	interstitial = [[GADInterstitial alloc] initWithAdUnitID:@"ca-app-pub-5564002345241286/1715397850"];
	interstitial.delegate = ad_delegate;
	GADRequest *request = [GADRequest request];
	// Request test ads on devices you specify. Your test device ID is printed to the console when
	// an ad request is made.
	//request.testDevices = @[ kGADSimulatorID, @"FIXME-FOR-TESTING" ];
	[interstitial loadRequest:request];
	return NULL;
}

void requestNewInterstitial()
{
	if (purchased == 1) {
		return;
	}
	al_run_detached_thread(request_thread, NULL);
}

void showAd()
{
	if (purchased == 1) {
		return;
	}

	ALLEGRO_DISPLAY *display = al_get_current_display();

	[[NSOperationQueue mainQueue] addOperationWithBlock:^{
		if (interstitial.isReady) {
			[interstitial presentFromRootViewController:al_iphone_get_window(display).rootViewController];
			count = 0;
		}
		else {
			count++;
			if (count >= 3) {
				requestNewInterstitial();
				count = 0;
			}
		}
	}];
}

void do_alert(NSString *msg)
{
	UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Alert"
	message:msg
	preferredStyle:UIAlertControllerStyleAlert];

	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
	handler:^(UIAlertAction * action) {}];

	[alert addAction:defaultAction];
	[[al_iphone_get_window(allegro_display) rootViewController] presentViewController:alert animated:YES completion:nil];
}

@interface TransactionObserver<SKPaymentTransactionObserver> : NSObject
{
}
- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error;
- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue;
- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray<SKPaymentTransaction *> *)transactions;
- (void)paymentQueue:(SKPaymentQueue *)queue removedTransactions:(NSArray<SKPaymentTransaction *> *)transactions;
@end

@implementation TransactionObserver
- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error
{
	queried_purchased = 0;
}
- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue
{
	NSArray<SKPaymentTransaction *> *transactions = [queue transactions];
    if ([transactions count] == 0) {
        queried_purchased = 0;
        return;
    }
	for (int i = 0; i < [transactions count]; i++) {
		SKPaymentTransaction *t = [transactions objectAtIndex:i];
		if ([[[t payment] productIdentifier] isEqual:@"noads"]) {
			if ([t transactionState] == SKPaymentTransactionStatePurchased || [t transactionState] == SKPaymentTransactionStateRestored) {
				queried_purchased = 1;
			}
			else if ([t transactionState] == SKPaymentTransactionStateFailed) {
				queried_purchased = 0;
			}
			// else, wait for the final verdict
		}
	}
}
- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray<SKPaymentTransaction *> *)transactions;
{
	for (int i = 0; i < [transactions count]; i++) {
		SKPaymentTransaction *t = [transactions objectAtIndex:i];
		if ([[[t payment] productIdentifier] isEqual:@"noads"]) {
			if ([t transactionState] == SKPaymentTransactionStatePurchased || [t transactionState] == SKPaymentTransactionStateRestored) {
				pay_purchased = 1;
				[[SKPaymentQueue defaultQueue] finishTransaction:t];
			}
			else if ([t transactionState] == SKPaymentTransactionStateFailed) {
				NSLog([[t error] domain]);
				printf("%d\n", [[t error] code]);
				do_alert([[t error] localizedDescription]);
				pay_purchased = 0;
				NSLog(@"Transaction failed!");
				[[SKPaymentQueue defaultQueue] finishTransaction:t];
			}
			// else, wait for the final verdict
		}
		else {
			if ([t transactionState] == SKPaymentTransactionStatePurchased || [t transactionState] == SKPaymentTransactionStateRestored || [t transactionState] == SKPaymentTransactionStateFailed) {
				[[SKPaymentQueue defaultQueue] finishTransaction:t];
			}
		}
	}
}
- (void)paymentQueue:(SKPaymentQueue *)queue removedTransactions:(NSArray<SKPaymentTransaction *> *)transactions
{
}
@end

@interface ProductRequestDelegate<SKProductsRequestDelegate> : NSObject
{
}
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response;
@end

static TransactionObserver<SKPaymentTransactionObserver> *observer;

@implementation ProductRequestDelegate
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response;
{
	if ([[response invalidProductIdentifiers] count] > 0) {
		do_alert(@"Invalid product identifier");
        NSLog(@"Invalid product identifier!");
		pay_purchased = 0;
		return;
	}
	for (int i = 0; i < [[response products] count]; i++) {
		SKProduct *p = [[response products] objectAtIndex:i];
		if ([[p productIdentifier] isEqual:@"noads"]) {
			if (pay_purchased == -1) {
				SKPayment *payment = [SKPayment paymentWithProduct:p];
				if (observer == NULL) {
					observer = [[TransactionObserver<SKPaymentTransactionObserver> alloc] init];
					[[SKPaymentQueue defaultQueue] addTransactionObserver:observer];
				}
				[[SKPaymentQueue defaultQueue] addPayment:payment];
				return;
			}
		}
		else {
            NSLog(@"Unknown product identifier!");
			pay_purchased = 0;
		}
	}
    NSLog(@"No products!");
	pay_purchased = 0;
}
@end

void queryPurchased()
{
}

static void queryPurchased_real()
{
	queried_purchased = -1;
	if (observer == NULL) {
		observer = [[TransactionObserver<SKPaymentTransactionObserver> alloc] init];
		[[SKPaymentQueue defaultQueue] addTransactionObserver:observer];
	}
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

static ProductRequestDelegate<SKProductsRequestDelegate> *my_delegate;

void restore_purchases()
{
	queryPurchased_real();
	while (queried_purchased == -1); // wait
	if (queried_purchased == 1) {
		purchased = 1;
		really_checking_purchase = true;
		return;
	}
}

void doIAP()
{
	allegro_display = al_get_current_display();

	Reachability *networkReachability = [Reachability reachabilityForInternetConnection];   
	NetworkStatus networkStatus = [networkReachability currentReachabilityStatus];    
	if (networkStatus == NotReachable) {        
		show_please_connect_dialog(false);
		return;
	}

	restore_purchases();

	if (purchased == 1) {
		return;
	}

	pay_purchased = -1;
	NSArray *products_a = @[@"noads"];
	NSSet *products = [NSSet setWithArray:products_a];
	SKProductsRequest *request = [[SKProductsRequest alloc] initWithProductIdentifiers:products];
	my_delegate = [[ProductRequestDelegate<SKProductsRequestDelegate> alloc] init];
	request.delegate = my_delegate;
	[request start];
	while (pay_purchased == -1); // wait
	purchased = pay_purchased;
	if (pay_purchased == 1) {
		really_checking_purchase = true;
	}
}

int checkPurchased()
{
	return isPurchased();
}

int isPurchased()
{
	if (really_checking_purchase) {
		return purchased;
	}
	else {
		return isPurchased_engine();
	}
}
#endif // ADMOB

void LOG(const char *s)
{
	NSLog(@"%s", s);
}

// localization

const char *get_apple_language()
{
	static char buf[100];

    NSString *str = [[NSLocale preferredLanguages] objectAtIndex:0];
    
	if ([str hasPrefix:@"de"]) {
		strcpy(buf, "german");
	}
	else if ([str hasPrefix:@"fr"]) {
		strcpy(buf, "french");
	}
	else if ([str hasPrefix:@"ko"]) {
		strcpy(buf, "korean");
	}
	else {
		strcpy(buf, "english");
	}

	return buf;
}

#ifdef ALLEGRO_IPHONE
//#include <allegro5/allegro_iphone.h>
//#include <allegro5/allegro_iphone_objc.h>

static bool license_done;

static void all_done()
{
	UIViewController *root = [[[UIApplication sharedApplication] keyWindow] rootViewController];
	[root dismissViewControllerAnimated:YES completion:nil];
	license_done = true;
}

@interface MyTextView : UITextView
{
}
@end

@implementation MyTextView
@end

@interface LicenseViewController : UIViewController
{
	UIViewController *parent;
    NSAttributedString *txt;
    MyTextView *text_view;
}
- (id)initWithHTML:(NSAttributedString *)text;
- (void)done;
- (void)go:(UINavigationController *)nav;
- (void)createTextView:(NSObject *)unused;
- (void) viewDidAppear:(BOOL)animated;
@end

@implementation LicenseViewController
- (void) createTextView:(NSObject *)unused
{
    CGRect f;
    f.origin.x = 0;
    f.origin.y = 0;
    f.size.width = 0;
    f.size.height = 0;
    MyTextView *text_view = [[MyTextView alloc] initWithFrame:f];
    text_view.attributedText = txt;
    text_view.editable = FALSE;
    SEL selector = NSSelectorFromString(@"setSelectable:");
    if ([text_view respondsToSelector:selector]) {
        text_view.selectable = TRUE;
    }
    text_view.userInteractionEnabled = TRUE;

    self.view = text_view;
}
- (void) viewDidAppear:(BOOL)animated
{
    UIBarButtonItem *bbi = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(done)];
    [self.navigationItem setRightBarButtonItem:bbi animated:NO];
}

- (id) initWithHTML:(NSAttributedString *)text
{
	self = [super initWithNibName:nil bundle:nil];
    
    txt = text;

	// Size doesn't seem to matter...
    [self performSelectorOnMainThread:@selector(createTextView:) withObject:nil waitUntilDone:YES];
	//text_view.attributedText = text;

    //UIBarButtonItem *bbi = [[UIBarButtonItem alloc] initWithTitle:@"Back" style:UIBarButtonItemStylePlain target:self action:@selector(done)];
    //self.navigationItem.backBarButtonItem = bbi;
    //[self.navigationItem setHidesBackButton:NO animated:NO];

    SEL selector2 = NSSelectorFromString(@"setEdgesForExtendedLayout:");
    if ([self respondsToSelector:selector2]) {
        self.edgesForExtendedLayout = UIRectEdgeNone;
    }

	return self;
}
- (void)done
{
	all_done();
}
- (void)go:(UINavigationController *)nav
{
    [[al_iphone_get_window(allegro_display) rootViewController] presentViewController:nav animated:YES completion:nil];
}
@end

bool ios_show_text(std::string fn_s)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSString *text = @"Crystal Picnic - 3rd Party Licenses\n" \
"\n" \
"Various open source and commercial libraries are used by Crystal Picnic, including Allegro, BASS, FreeType, libpng, Lua, Poly2Tri and zlib.\n" \
"\n" \
"--\n" \
"\n" \
"Allegro includes some files from bstrlib under the following terms.\n" \
"\n" \
"Copyright (c) 2002-2008 Paul Hsieh All rights reserved.\n" \
"\n" \
"Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:\n" \
"\n" \
"Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.\n" \
"\n" \
"Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.\n" \
"\n" \
"Neither the name of bstrlib nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.\n" \
"\n" \
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n" \
"\n" \
"--\n" \
"\n" \
"Lua is used under the following license:\n" \
"\n" \
"Copyright (c) 1994-2015 Lua.org, PUC-Rio.\n" \
"\n" \
"Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n" \
"\n" \
"The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n" \
"\n" \
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n" \
"\n" \
"--\n" \
"\n" \
"Portions of this software are copyright © 2018 The FreeType Project\n" \
"(www.freetype.org). All rights reserved.\n" \
"\n" \
"--\n" \
"\n" \
"The Mersenne Twister implementation in src/mt19937ar.c is used under the following license:\n" \
"\n" \
"Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura, All rights reserved.                    \n" \
"Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:\n" \
"\n" \
"1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.\n" \
"\n" \
"2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.\n" \
"\n" \
"3. The names of its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.\n" \
"\n" \
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n" \
"\n" \
"--\n" \
"\n" \
"Poly2Tri is used under the following license:\n" \
"\n" \
"Poly2Tri Copyright (c) 2009-2010, Poly2Tri Contributors\n" \
"\n" \
"All rights reserved.\n" \
"\n" \
"Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:\n" \
"\n" \
"Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.\n" \
"\n" \
"Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.\n" \
"\n" \
"Neither the name of Poly2Tri nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.\n" \
"\n" \
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n" \
"\n" \
"--\n" \
"\n" \
"The Korean font (NanumGothicBold) is used under the following license:\n" \
"\n" \
"Copyright (c) 2010, NHN Corporation (http://www.nhncorp.com), with Reserved Font Name Nanum, Naver Nanum, NanumGothic, Naver NanumGothic, NanumMyeongjo, Naver NanumMyeongjo, NanumBrush, Naver NanumBrush, NanumPen, Naver NanumPen.\n" \
"\n" \
"This Font Software is licensed under the SIL Open Font License, Version 1.1.  This license is copied below, and is also available with a FAQ at: http://scripts.sil.org/OFL\n" \
"\n" \
"SIL OPEN FONT LICENSE Version 1.1 - 26 February 2007\n" \
"\n" \
"PREAMBLE\n" \
"\n" \
"The goals of the Open Font License (OFL) are to stimulate worldwide development of collaborative font projects, to support the font creation efforts of academic and linguistic communities, and to provide a free and open framework in which fonts may be shared and improved in partnership with others.\n" \
"\n" \
"The OFL allows the licensed fonts to be used, studied, modified and redistributed freely as long as they are not sold by themselves. The fonts, including any derivative works, can be bundled, embedded, redistributed and/or sold with any software provided that any reserved names are not used by derivative works. The fonts and derivatives, however, cannot be released under any other type of license. The requirement for fonts to remain under this license does not apply to any document created using the fonts or their derivatives.\n" \
"\n" \
"DEFINITIONS\n" \
"\n" \
"\"Font Software\" refers to the set of files released by the Copyright Holder(s) under this license and clearly marked as such. This may include source files, build scripts and documentation.\n" \
"\n" \
"\"Reserved Font Name\" refers to any names specified as such after the copyright statement(s).\n" \
"\n" \
"\"Original Version\" refers to the collection of Font Software components as distributed by the Copyright Holder(s).\n" \
"\n" \
"\"Modified Version\" refers to any derivative made by adding to, deleting, or substituting -- in part or in whole -- any of the components of the Original Version, by changing formats or by porting the Font Software to a new environment.\n" \
"\n" \
"\"Author\" refers to any designer, engineer, programmer, technical writer or other person who contributed to the Font Software.\n" \
"\n" \
"PERMISSION & CONDITIONS\n" \
"\n" \
"Permission is hereby granted, free of charge, to any person obtaining a copy of the Font Software, to use, study, copy, merge, embed, modify, redistribute, and sell modified and unmodified copies of the Font Software, subject to the following conditions:\n" \
"\n" \
"1) Neither the Font Software nor any of its individual components, in Original or Modified Versions, may be sold by itself.\n" \
"\n" \
"2) Original or Modified Versions of the Font Software may be bundled, redistributed and/or sold with any software, provided that each copy contains the above copyright notice and this license. These can be included either as stand-alone text files, human-readable headers or in the appropriate machine-readable metadata fields within text or binary files as long as those fields can be easily viewed by the user.\n" \
"\n" \
"3) No Modified Version of the Font Software may use the Reserved Font Name(s) unless explicit written permission is granted by the corresponding Copyright Holder. This restriction only applies to the primary font name as presented to the users.\n" \
"\n" \
"4) The name(s) of the Copyright Holder(s) or the Author(s) of the Font Software shall not be used to promote, endorse or advertise any Modified Version, except to acknowledge the contribution(s) of the Copyright Holder(s) and the Author(s) or with their explicit written permission.\n" \
"\n" \
"5) The Font Software, modified or unmodified, in part or in whole, must be distributed entirely under this license, and must not be distributed under any other license. The requirement for fonts to remain under this license does not apply to any document created using the Font Software.\n" \
"\n" \
"TERMINATION\n" \
"\n" \
"This license becomes null and void if any of the above conditions are not met.\n" \
"\n" \
"DISCLAIMER\n" \
"\n" \
"THE FONT SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF COPYRIGHT, PATENT, TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, INCLUDING ANY GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF THE USE OR INABILITY TO USE THE FONT SOFTWARE OR FROM OTHER DEALINGS IN THE FONT SOFTWARE.\n" \
"\n" \
"--\n" \
"\n" \
"Windows crash dump generation is taken from Aseprite and is used under the following license:\n" \
"\n" \
"Copyright (c) 2001-2015 David Capello\n" \
"\n" \
"Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n" \
"\n" \
"The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n" \
"\n" \
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.";
    
	// it's not HTML here, not supported on iOS 6
	NSAttributedString *html = [[NSAttributedString alloc] initWithString:text];
	if (html == nil) {
		[pool release];
		return false;
	}

	license_done = false;

    LicenseViewController *license_vc = [[LicenseViewController alloc] initWithHTML:html];

	UINavigationController *nav = [[UINavigationController alloc] initWithRootViewController:license_vc];
    
    [license_vc performSelectorOnMainThread:@selector(go:) withObject:nav waitUntilDone:YES];

	//UIViewController *root = [[[UIApplication sharedApplication] keyWindow] rootViewController];

	//[root presentViewController:nav animated:YES completion:nil];

	/*
	while (license_done == false) {
		SDL_PumpEvents();
		SDL_Delay(1);
	}

	SDL_SetHint(SDL_HINT_APPLE_TV_CONTROLLER_UI_EVENTS, "0");
	
	SDL_PumpEvents();
	SDL_FlushEvents(0, 0xffffffff);
	*/

	[pool release];

	return true;
}

bool ios_show_license()
{
    allegro_display = al_get_current_display();
    bool res = ios_show_text("3rd_party");
    return res;
}
#endif

#ifdef ALLEGRO_MACOSX
void macosx_open_with_system(std::string filename)
{
	[[NSWorkspace sharedWorkspace] openFile:[NSString stringWithUTF8String:filename.c_str()]];
}
#endif
