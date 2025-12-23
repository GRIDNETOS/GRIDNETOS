#include "CVMMetaParser.h"
#include "VMMetaSection.h"
#include "VMMetaEntry.h"

CVMMetaParser::CVMMetaParser()
{
    mVersion = 1;
    mSuppSectionsVersion = 2;
}

std::vector<std::shared_ptr<CVMMetaEntry>> CVMMetaParser::getEntriesByRequestID(uint64_t id, eVMMetaEntryType::eVMMetaEntryType eType)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    std::vector <std::shared_ptr<CVMMetaEntry>> toRet;
    if (mSections.size() == 0)
        return toRet;

   

    eVMMetaSectionType::eVMMetaSectionType searchedSectionType = (eVMMetaEntryType::dataRequest ? eVMMetaSectionType::requests : eVMMetaSectionType::notifications);

    for (int i = 0; i < mSections.size(); i++)
    {
        if (mSections[i]->getType() == searchedSectionType)
        {
            for (int a = 0; a < mSections[i]->getEntries().size(); a++)
            {
                if (mSections[i]->getEntries()[a]->getReqID() == id)
                {
                    toRet.push_back(mSections[i]->getEntries()[a]);
                }
                    
            }
        }
    }
    return toRet;
}

std::vector<std::shared_ptr<CVMMetaSection>> CVMMetaParser::decode(std::vector<uint8_t> data)
{
    std::lock_guard<std::mutex> lock(mGuardian);
    std::vector<std::shared_ptr<CVMMetaSection>> sections; 
    size_t metaVersion = 0;
    try {
        if (data.size() == 0)
            return sections;

        Botan::BER_Decoder dec = Botan::BER_Decoder(data).start_cons(Botan::ASN1_Tag::SEQUENCE).decode(metaVersion);//attempt to decode header
        
        if (metaVersion == mVersion)
        {
            Botan::BER_Decoder wrapperSequence = Botan::BER_Decoder(dec);

            while (wrapperSequence.more_items())//traverse sections
            {
                Botan::BER_Object obj = wrapperSequence.get_next_object();//get next section
                if (obj.type_tag != Botan::ASN1_Tag::SEQUENCE)
                    throw "error";

                Botan::BER_Decoder sectionSequence = Botan::BER_Decoder(obj.value);
                size_t version, sectionType;
                version = 0; sectionType = 0;
                std::vector<uint8_t> metadata;
                sectionSequence.decode(version);
                if (version <= mSuppSectionsVersion)
                {
                    sectionSequence.decode(sectionType);
                    sectionSequence.decode(metadata, Botan::ASN1_Tag::OCTET_STRING);
                    std::shared_ptr<CVMMetaSection> section = std::make_shared<CVMMetaSection>(static_cast<eVMMetaSectionType::eVMMetaSectionType>(
                        sectionType), version);
                    section->setMetaData(metadata);

                    obj = sectionSequence.get_next_object();//get entries-sequence
                    if (obj.type_tag != Botan::ASN1_Tag::SEQUENCE)
                        throw "error";


                    Botan::BER_Decoder  entriesSequence = Botan::BER_Decoder(obj.value);//decode traverse entry-sequences
                    size_t entryType, reqID, appID;
                    std::vector<uint8_t> vmID, tempVMID;
                    
                    while (entriesSequence.more_items())
                    {
                        entryType = reqID = appID = 0;//reset variables
                        Botan::BER_Object obj2 = entriesSequence.get_next_object();
                        if (obj2.type_tag != Botan::ASN1_Tag::SEQUENCE)
                            throw "error";

                        Botan::BER_Decoder entrySequence = Botan::BER_Decoder(obj2.value);

                      
                        entrySequence.decode(entryType);//in case of requests
                        entrySequence.decode(reqID);
                        entrySequence.decode(appID);
                        entrySequence.decode(tempVMID, Botan::ASN1_Tag::OCTET_STRING);
                        if (tempVMID.size() > 0)
                        {
                            vmID = tempVMID;//only if a value provided the value affects the current entry.
                            //if no value provided the previous VM is to be used. that's for efficiency.
                        }


                        obj = entrySequence.get_next_object();//get data-fields sequence
                        if (obj.type_tag != Botan::ASN1_Tag::SEQUENCE)
                            throw "error";

                        std::shared_ptr <CVMMetaEntry> entry = std::make_shared<CVMMetaEntry>(static_cast<eVMMetaEntryType::eVMMetaEntryType>(entryType), 1, reqID, appID, vmID);
                        Botan::BER_Decoder  dataFieldsSequence = Botan::BER_Decoder(obj.value);//decode section entry

                        while (dataFieldsSequence.more_items())
                        {
                            std::vector<uint8_t> d = Botan::unlock(dataFieldsSequence.get_next_object().value);//note this is immue to a particular data type ex.  OCTET_STRING vs INTEGER
                            entry->addField(d);
                        }

                        //include Entry into Section
                        section->addEntry(entry);
                    }
                    sections.push_back(section);
                }
                else return sections;

            }
            mSections = sections;
        }
        return sections;
       
    }
    catch (...)
    {
        return std::vector<std::shared_ptr<CVMMetaSection>>();
    }
    
}

void CVMMetaParser::reset()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    mSections.clear();
}

uint64_t CVMMetaParser::getVersion()
{
    std::lock_guard<std::mutex> lock(mGuardian);
    return mVersion;
}
