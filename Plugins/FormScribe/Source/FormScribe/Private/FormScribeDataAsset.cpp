#include "FormScribeDataAsset.h"

#if WITH_EDITOR
void UFormScribeDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	OnDataChanged.Broadcast();
}
#endif
