//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsISDBAccessControlDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"ISDB_access_control_descriptor"
#define MY_CLASS ts::ISDBAccessControlDescriptor
#define MY_DID ts::DID_ISDB_CA
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::STD_ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBAccessControlDescriptor::ISDBAccessControlDescriptor(uint16_t id, PID p) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    CA_system_id(id),
    transmission_type(7), // broadcast route
    pid(p),
    private_data()
{
    _is_valid = true;
}

ts::ISDBAccessControlDescriptor::ISDBAccessControlDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBAccessControlDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBAccessControlDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(CA_system_id);
    bbp->appendUInt16(uint16_t(uint16_t(transmission_type & 0x07) << 13) | pid);
    bbp->append(private_data);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBAccessControlDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 4;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        CA_system_id = GetUInt16(data);
        transmission_type = (data[2] >> 5) & 0x07;
        pid = GetUInt16(data + 2) & 0x1FFF;
        private_data.copy(data + 4, size - 4);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBAccessControlDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    if (size < 4) {
        display.displayExtraData(data, size, indent);
    }
    else {
        DuckContext& duck(display.duck());
        std::ostream& strm(duck.out());
        const std::string margin(indent, ' ');

        // Extract common part
        const uint16_t casid = GetUInt16(data);
        const uint8_t type = (data[2] >> 5) & 0x07;
        uint16_t pid = GetUInt16(data + 2) & 0x1FFF;
        const UChar* const dtype = tid == TID_CAT ? u"EMM" : (tid == TID_PMT ? u"ECM" : u"CA");
        data += 4; size -= 4;

        strm << margin << "CA System Id: " << names::CASId(duck, casid, names::FIRST) << std::endl
             << margin << "Transmission type: " << NameFromSection(u"ISDBCATransmissionType", type, names::DECIMAL_FIRST) << std::endl
             << margin << UString::Format(u"%s PID: 0x%X (%d)", {dtype, pid, pid}) << std::endl;

        display.displayPrivateData(u"Private CA data", data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBAccessControlDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CA_system_id", CA_system_id, true);
    root->setIntAttribute(u"transmission_type", transmission_type);
    root->setIntAttribute(u"PID", pid, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::ISDBAccessControlDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint16_t>(CA_system_id, u"CA_system_id", true) &&
        element->getIntAttribute<uint8_t>(transmission_type, u"transmission_type", false, 7, 0, 7) &&
        element->getIntAttribute<PID>(pid, u"PID", true, 0, 0x0000, 0x1FFF) &&
        element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 4);
}
