#pragma once
// SOURCE ONLY: private receipt sink, not a writer or registered callback.
#include "fire_native_published_use.h"
#include "fire_original_construct_producer.h"
namespace FirePersonalOccurrenceSource {
class PublishedNativeUseSink final {
public:
 static void Receive(Client&,PublishedNativeUse&);
};
} // namespace FirePersonalOccurrenceSource
