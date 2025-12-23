
#include "SearchResults.hpp"
#include "TransactionDesc.hpp"
#include "BlockDesc.hpp"
#include "DomainDesc.hpp"
#include "Tools.h"

CSearchResults::CSearchResults(const std::vector<ResultData>& results, uint64_t totalCount, uint64_t currentPage, uint64_t itemsPerPage)
    : mResults(results), mTotalCount(totalCount), mCurrentPage(currentPage), mItemsPerPage(itemsPerPage) {
    validateInputs();
}

void CSearchResults::validateInputs() {
    if (mTotalCount < 0) {
        throw std::invalid_argument("Total count must be a non-negative integer");
    }
    if (mCurrentPage < 1) {
        throw std::invalid_argument("Current page must be a positive integer");
    }
    if (mItemsPerPage < 1) {
        throw std::invalid_argument("Items per page must be a positive integer");
    }
}

bool CSearchResults::getPackedData(std::vector<uint8_t>& packedData) const {
    std::lock_guard<std::mutex> lock(mGuardian);
    try {
        std::shared_ptr<CTools> tools = CTools::getInstance();
        Botan::DER_Encoder enc;
        enc.start_cons(Botan::ASN1_Tag::SEQUENCE)
            .encode(mTotalCount)
            .encode(mCurrentPage)
            .encode(mItemsPerPage);
        enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
        for (const auto& result : mResults) {
            bool success = std::visit([&](const auto& ptr) -> bool {
                using T = std::decay_t<decltype(ptr)>;
                if constexpr (std::is_same_v<T, std::nullptr_t>) {
                    // Skip null entries
                    return true;
                }
                else if constexpr (std::is_same_v<T, std::shared_ptr<CTransactionDesc>> ||
                    std::is_same_v<T, std::shared_ptr<CBlockDesc>> ||
                    std::is_same_v<T, std::shared_ptr<CDomainDesc>>) {
                    enc.start_cons(Botan::ASN1_Tag::SEQUENCE);
                    if constexpr (std::is_same_v<T, std::shared_ptr<CTransactionDesc>>) {
                        enc.encode(static_cast<uint64_t>(eSearchResultElemType::transaction));
                    }
                    else if constexpr (std::is_same_v<T, std::shared_ptr<CBlockDesc>>) {
                        enc.encode(static_cast<uint64_t>(eSearchResultElemType::block));
                    }
                    else if constexpr (std::is_same_v<T, std::shared_ptr<CDomainDesc>>) {
                        enc.encode(static_cast<uint64_t>(eSearchResultElemType::domain));
                    }
                    std::vector<uint8_t> packedEntry;
                    if (!ptr->getPackedData(packedEntry)) {
                        return false;
                    }
                    enc.encode(packedEntry, Botan::ASN1_Tag::OCTET_STRING);
                    enc.end_cons();
                    return true;
                }
                else {
                    return false;
                }
                }, result);
            if (!success) {
                return false;
            }
        }
        enc.end_cons().end_cons();
        packedData = enc.get_contents_unlocked();
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

uint64_t CSearchResults::getResultTimestamp(const ResultData& result) {
    return std::visit([](const auto& ptr) -> uint64_t {
        using T = std::decay_t<decltype(ptr)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return 0; // Return 0 for null entries
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<CTransactionDesc>>) {
            if (ptr) {
                uint64_t confirmedTimestamp = ptr->getConfirmedTimestamp();
                uint64_t unconfirmedTimestamp = ptr->getUnconfirmedTimestamp();
                return confirmedTimestamp > 0 ? confirmedTimestamp : unconfirmedTimestamp;
            }
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<CBlockDesc>>) {
            if (ptr) {
                return ptr->getSolvedAt();
            }
        }
        else if constexpr (std::is_same_v<T, std::shared_ptr<CDomainDesc>>) {
            if (ptr) {
                return std::time(0); // domains would always display first
                // todo: Implement getLastUpdated() in CDomainDesc
            }
        }
        return 0; // Return 0 for any unhandled cases or null shared_ptrs
        }, result);
}


std::shared_ptr<CSearchResults> CSearchResults::instantiate(const std::vector<uint8_t>& packedData) {
    try {
        std::shared_ptr<CTools> tools = CTools::getInstance();
        Botan::BER_Decoder dec(packedData);
        uint64_t totalCount, currentPage, itemsPerPage;
        std::vector<ResultData> results;

        dec.start_cons(Botan::ASN1_Tag::SEQUENCE)
            .decode(totalCount)
            .decode(currentPage)
            .decode(itemsPerPage);

        Botan::BER_Decoder resultsDec = dec.start_cons(Botan::ASN1_Tag::SEQUENCE);
        while (resultsDec.more_items()) {
            Botan::BER_Decoder itemDec = resultsDec.start_cons(Botan::ASN1_Tag::SEQUENCE);
            uint64_t typeValue;
            std::vector<uint8_t> itemData;
            itemDec.decode(typeValue)
                .decode(itemData, Botan::ASN1_Tag::OCTET_STRING)
                .end_cons();

            eSearchResultElemType::eSearchResultElemType type = static_cast<eSearchResultElemType::eSearchResultElemType>(typeValue);

            std::shared_ptr<CSearchResults::ResultData> item;
            switch (type) {
            case eSearchResultElemType::transaction:
                item = std::make_shared<CSearchResults::ResultData>(CTransactionDesc::instantiate(itemData));
                break;
            case eSearchResultElemType::block:
                item = std::make_shared<CSearchResults::ResultData>(CBlockDesc::instantiate(itemData));
                break;
            case eSearchResultElemType::domain:
                item = std::make_shared<CSearchResults::ResultData>(CDomainDesc::instantiate(itemData));
                break;
            default:
                return nullptr;
            }
            results.push_back(std::move(*item));
        }
        resultsDec.end_cons();
        dec.end_cons();

        return std::make_shared<CSearchResults>(results, totalCount, currentPage, itemsPerPage);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("Deserialization failed: ") + e.what());
    }
}

uint64_t CSearchResults::getResultCount() const {
    return mResults.size();
}

uint64_t CSearchResults::getTotalResultCount() const {
    return mTotalCount;
}

uint64_t CSearchResults::getCurrentPage() const {
    return mCurrentPage;
}

uint64_t CSearchResults::getItemsPerPage() const {
    return mItemsPerPage;
}

std::vector<CSearchResults::ResultData> CSearchResults::getAllResults() const {
    return mResults;
}

std::vector<CSearchResults::ResultData> CSearchResults::getResultsByType(eSearchResultElemType::eSearchResultElemType type) const {
    std::vector<ResultData> filteredResults;
    std::copy_if(mResults.begin(), mResults.end(), std::back_inserter(filteredResults),
        [type](const ResultData& item) { return item.index() == static_cast<size_t>(type); });
    return filteredResults;
}

CSearchResults::ResultData CSearchResults::getResult(size_t index) const {
    if (index >= mResults.size()) {
        throw std::out_of_range("Result index out of range");
    }
    return mResults[index];
}

// Iterator implementation
CSearchResults::iterator CSearchResults::begin() {
    return mResults.begin();
}

CSearchResults::iterator CSearchResults::end() {
    return mResults.end();
}

CSearchResults::const_iterator CSearchResults::begin() const {
    return mResults.begin();
}

CSearchResults::const_iterator CSearchResults::end() const {
    return mResults.end();
}