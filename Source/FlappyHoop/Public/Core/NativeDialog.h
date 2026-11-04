#pragma once

#include "CoreMinimal.h"

/**
 * The phone's own message popup: AlertDialog on Android, UIAlertController on iOS.
 * Non-blocking (the game keeps running underneath) with a single button that just closes it.
 * Android: the Java side is in FlappyHoop_UPL.xml (AndroidThunkJava_FlappyShowMessage).
 */
namespace FlappyNativeDialog
{
	/** True on Android and iOS. Elsewhere Show() only logs, so show your own UI instead. */
	FLAPPYHOOP_API bool IsSupported();

	FLAPPYHOOP_API void Show(const FText& Title, const FText& Message, const FText& ButtonText);
}
