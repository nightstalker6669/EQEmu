#pragma once
#include <cstdint>
#include <memory>
#include <optional>
class Client;
namespace FirePersonalOccurrenceSource {class NativeItemCustody;}
namespace FireRunActionSource {
class OriginalMoldTrade;
class PreparedTerminalChestPlan;
class OriginalTerminalDeathProof;
class NativeTerminalPublication;
class NativeRunTerminalSource;
class PublishedTerminalReturnSink;
enum class OriginalTerminalReturnResult;
struct PublishedTerminalReturnView {
 const OriginalMoldTrade* original=nullptr;
 PreparedTerminalChestPlan* plan=nullptr;
 uint64_t terminal_row=0,run_id=0,native_sequence=0;
};
class PublishedTerminalReturnReceipt final {
 friend class FirePersonalOccurrenceSource::NativeItemCustody;
 struct State;
 std::unique_ptr<State> state_;
 explicit PublishedTerminalReturnReceipt(std::unique_ptr<State>)noexcept;
public:
 ~PublishedTerminalReturnReceipt();
 PublishedTerminalReturnReceipt(PublishedTerminalReturnReceipt&&)noexcept;
 PublishedTerminalReturnReceipt(const PublishedTerminalReturnReceipt&)=delete;
 PublishedTerminalReturnReceipt& operator=(const PublishedTerminalReturnReceipt&)=delete;
 PublishedTerminalReturnReceipt& operator=(PublishedTerminalReturnReceipt&&)=delete;
 std::optional<PublishedTerminalReturnView> Take(const Client&)noexcept;
};
class NativeRunTerminalCustody final {
 friend class NativeRunTerminalSource;
 friend class FirePersonalOccurrenceSource::NativeItemCustody;
 static bool ValidateOriginalMoldReturn(Client&,const OriginalMoldTrade&);
 static OriginalTerminalReturnResult ExecuteOriginalReturn(Client&,
  std::unique_ptr<OriginalMoldTrade>,std::unique_ptr<PreparedTerminalChestPlan>);
 static bool AcknowledgeOriginalDeath(OriginalTerminalDeathProof&);
 static bool AcknowledgeNativePublication(NativeTerminalPublication&);
};
void OnOriginalTerminalReturnPublished(Client&,PublishedTerminalReturnReceipt&);
} // namespace FireRunActionSource
