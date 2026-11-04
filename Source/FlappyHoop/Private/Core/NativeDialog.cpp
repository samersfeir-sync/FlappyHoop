#include "Core/NativeDialog.h"
#include "Core/FlappyHoop.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Android/AndroidJavaEnv.h"
#endif

#if PLATFORM_IOS
#import <UIKit/UIKit.h>
#endif

namespace FlappyNativeDialog
{
#if PLATFORM_IOS
	/** The view controller on top (an alert must be presented from the top-most one). Main thread only. */
	static UIViewController* GetTopViewController()
	{
		UIViewController* Top = nil;
		for (UIScene* Scene in [UIApplication sharedApplication].connectedScenes)
		{
			if (![Scene isKindOfClass:[UIWindowScene class]])
			{
				continue;
			}
			for (UIWindow* Window in ((UIWindowScene*)Scene).windows)
			{
				if (Window.isKeyWindow)
				{
					Top = Window.rootViewController;
					break;
				}
			}
			if (Top)
			{
				break;
			}
		}

		while (Top && Top.presentedViewController)
		{
			Top = Top.presentedViewController;
		}
		return Top;
	}
#endif

	bool IsSupported()
	{
#if PLATFORM_ANDROID || PLATFORM_IOS
		return true;
#else
		return false;
#endif
	}

	void Show(const FText& Title, const FText& Message, const FText& ButtonText)
	{
		UE_LOG(LogFlappyHoop, Log, TEXT("[Dialog] %s: %s"), *Title.ToString(), *Message.ToString());

#if PLATFORM_ANDROID
		JNIEnv* Env = FAndroidApplication::GetJavaEnv();
		static jmethodID ShowMethod = Env ? FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID,
			"AndroidThunkJava_FlappyShowMessage", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V", false) : nullptr;
		if (!Env || !ShowMethod)
		{
			UE_LOG(LogFlappyHoop, Warning, TEXT("[Dialog] AndroidThunkJava_FlappyShowMessage missing (FlappyHoop_UPL.xml not applied?)"));
			return;
		}

		auto JTitle = FJavaHelper::ToJavaString(Env, Title.ToString());
		auto JMessage = FJavaHelper::ToJavaString(Env, Message.ToString());
		auto JButton = FJavaHelper::ToJavaString(Env, ButtonText.ToString());
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, ShowMethod, *JTitle, *JMessage, *JButton);

#elif PLATFORM_IOS
		NSString* TitleString = [Title.ToString().GetNSString() retain];
		NSString* MessageString = [Message.ToString().GetNSString() retain];
		NSString* ButtonString = [ButtonText.ToString().GetNSString() retain];

		dispatch_async(dispatch_get_main_queue(), ^{
			UIAlertController* Alert = [UIAlertController alertControllerWithTitle:TitleString
			                                                               message:MessageString
			                                                        preferredStyle:UIAlertControllerStyleAlert];
			[Alert addAction:[UIAlertAction actionWithTitle:ButtonString style:UIAlertActionStyleDefault handler:nil]];

			if (UIViewController* Top = GetTopViewController())
			{
				[Top presentViewController:Alert animated:YES completion:nil];
			}

			[TitleString release];
			[MessageString release];
			[ButtonString release];
		});
#endif
	}
}
