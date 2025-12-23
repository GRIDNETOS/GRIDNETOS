#include "QRIntent.h"

#include <string>
#include "QR/QrCode.hpp"

#include "BlockchainManager.h"
using namespace qrcodegen;
void CQRIntent::initFields()
{
	reinterpret_cast<uint8_t&>(mFlags) = 0;
	reinterpret_cast<uint8_t&>(mNFlags) = 0;
	mDestinationType = eEndpointType::WebSockConversation;
	mType = eQRIntentType::QRShow;
	mVersion = 1;
	mSourceSeq = 0;
	mOperationScope = eOperationScope::peer;
}
void CQRIntent::setSourceSeq(uint64_t seq)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mSourceSeq = seq;
}

void  CQRIntent::setOperationScope(eOperationScope::eOperationScope scope)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mOperationScope = scope;
}

eOperationScope::eOperationScope  CQRIntent::getOperationScope()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mOperationScope;
}


uint64_t CQRIntent::getSourceSeq()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mSourceSeq;
}
uint64_t CQRIntent::getVersion()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mVersion;
}

std::vector<uint8_t> CQRIntent::getChallange()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mChallange;
}

void  CQRIntent::setChallange(std::vector<uint8_t> data)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mChallange = data;
}


void CQRIntent::setNetworkFlags(nmFlags flags)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mNFlags = flags;
}

nmFlags CQRIntent::getNetworkFlags()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mNFlags;
}

void CQRIntent::setFlags(qrFlags flags)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mFlags = flags;
}
qrFlags CQRIntent::getFlags()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mFlags;
}


/// <summary>
/// Btrings an QRIntent to life.
/// </summary>
/// <param name="QRType"></param>
/// <returns></returns>
CQRIntent::CQRIntent(eQRIntentType::eQRIntentType QRType)
{
	initFields();
	mType = QRType;
}
CQRIntent::CQRIntent(eQRIntentType::eQRIntentType  QRType, std::vector<uint8_t> destinationID, std::vector<uint8_t> routeThrough,eEndpointType::eEndpointType eType )
{
	initFields();
	mType = QRType;
	mDestinationID = destinationID;
	mDestinationType = eType;
	mRouteThrough = routeThrough;
}

/// <summary>
/// Returns type of the QRIntent.
/// </summary>
/// <returns></returns>
eQRIntentType::eQRIntentType CQRIntent::getType()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mType;
}

/// <summary>
/// Sets QRIntent's type.
/// </summary>
/// <param name="QRType"></param>
void CQRIntent::setType(eQRIntentType::eQRIntentType QRType)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mType = QRType;
}

void CQRIntent::setPubKey(std::vector<uint8_t> pubKey)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mPubKey = pubKey;
}

std::vector<uint8_t> CQRIntent::getPubKey()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mPubKey;
}

void CQRIntent::setDestinationType(eEndpointType::eEndpointType eType)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mDestinationType = eType;
}

eEndpointType::eEndpointType CQRIntent::getDestinationType()
{
	return mDestinationType;
}

void CQRIntent::setRouteThrough(std::vector<uint8_t> ip)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mRouteThrough = ip;
}

std::vector<uint8_t> CQRIntent::getRouteThrough()
{
	return mRouteThrough;
}

/// <summary>
/// Sets data to store within the QR-code/intent.
/// </summary>
/// <param name="data"></param>
/// <returns></returns>
bool CQRIntent::setData(std::vector<uint8_t> data)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (data.size() > 2000)
		return false; //do not allow for more in the data-field itself. We need to account for BER-encoding overhead and additional fields.
	mData = data;
	return true;
}

/// <summary>
/// Retrieved main data from an QRIntent.
/// </summary>
/// <returns></returns>
std::vector<uint8_t> CQRIntent::getData()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mData;
}

/// <summary>
/// Sets sender info.
/// </summary>
/// <param name="senderinfo"></param>
/// <returns></returns>
bool CQRIntent::setInfo(std::vector<uint8_t> senderID)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	if (senderID.size() > 24)
		return false;

	mInfo = senderID;

	return true;
}

std::vector<uint8_t> CQRIntent::getInfo()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mInfo;
}

void CQRIntent::setDestinationID(std::vector<uint8_t> destinationID)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	mDestinationID = destinationID;
}

std::vector<uint8_t> CQRIntent::getDestinationID()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	return mDestinationID;
}

/// <summary>
/// Gets BER-encoded, serialized QRIntent data. This data will be directly  rendered as a QR-code.
/// </summary>
/// <returns></returns>
std::vector<uint8_t> CQRIntent::getPackedData()
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<uint8_t> flagsBytes(1), nflagsBytes(1);
	flagsBytes[0] = reinterpret_cast<uint8_t&>(mFlags);
	nflagsBytes[0] = reinterpret_cast<uint8_t&>(mNFlags);
	std::vector<uint8_t> dat;
	return Botan::DER_Encoder().start_cons(Botan::ASN1_Tag::SEQUENCE)
		.encode(static_cast<size_t>(mVersion))//subtype
		.encode(flagsBytes, Botan::ASN1_Tag::OCTET_STRING)
		.encode(nflagsBytes, Botan::ASN1_Tag::OCTET_STRING)
		.encode(static_cast<size_t>(mType))//subtype
		.encode(mData, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mDestinationID, Botan::ASN1_Tag::OCTET_STRING)
		.encode(static_cast<size_t>(mDestinationType))
		.encode(mRouteThrough, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mInfo, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mPubKey, Botan::ASN1_Tag::OCTET_STRING)
		.encode(mSourceSeq)
		.encode(mChallange, Botan::ASN1_Tag::OCTET_STRING)
		.encode(static_cast<size_t>(mOperationScope))
		.end_cons().get_contents_unlocked();
}

/// <summary>
/// Returns a QR-encoded matrix.
/// Note: if marginLength>0 the function provides also BOTH frame and margins (for in-terminal use).
/// No post processing thus needed.
/// </summary>
/// <returns></returns>
std::vector<std::vector<bool>> CQRIntent::getQRMatrix(uint64_t marginLength, bool doBottomMargin, bool doBottomFrame, uint64_t bottomMarginWidth)
{	
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::vector<std::vector<bool>> matrix;
	std::vector<uint8_t> packed = getPackedData();
	//https://stackoverflow.com/questions/37996101/storing-binary-data-in-qr-codes
	//lots of problems here with decoding of binary data using ZXing thus we additionaly encode the data with base64
	
	std::string txt = CTools::getInstance()->base58CheckEncode(packed);
	if (packed.size() == 0)
		return matrix;

	QrCode qr = QrCode::encodeBinary(std::vector<uint8_t>(txt.begin(),txt.end()), QrCode::Ecc::LOW);

	for (int i = 0; i < qr.getSize(); i++)
		matrix.push_back(std::vector<bool>(qr.getSize()));
	
	for (int y = 0; y < qr.getSize(); y++) {
		for (int x = 0; x < qr.getSize(); x++) {
			matrix[x][y] = qr.getModule(x, y);
		}
	}

	//TOP and BOTTOM margins - BEGIN
	if (marginLength)
	{	//(NOTE: a row in matrix is a COLUMN in View)
		//A column in matrix is thus a ROW in View.
		//Bits set to FALSE actually do shine.
	size_t frameLength = qr.getSize();
	frameLength += (marginLength || doBottomMargin)? (marginLength + (doBottomMargin ? bottomMarginWidth : 0) + 1 + (doBottomFrame ? 1 : 0)):0;
	std::vector<bool> vMarginOn(frameLength, true);//bits shining except for parts overlapping horizontal margins
	std::vector<bool> vMarginOff(frameLength, true);

	for (uint64_t i = 0; i < frameLength; i++)
	{
		if (i >= marginLength && i < (frameLength - marginLength))//prevent parts of TOP and BOTTOM margins from shining
		{//i.e. we're effectively introducing a vertical line.
			vMarginOn[i] = false;
		}
	}

	for (int i = 0; i < qr.getSize(); i++)
	{  //TOP and BOTTOM margins - BEGIN
		//here, we're effectively adding additional bit-fields to COLUMNS,
		//effectively resulting in additional ROWS being shown in View.

		//TOP Margin - BEGIN
		if (marginLength)
		{
			matrix[i].insert(matrix[i].begin(), false); //light it up (frame)
		}
		
		for (uint64_t c = 0; c < marginLength; c++)
		{
			matrix[i].insert(matrix[i].begin(), true);//dim it (black border)
		}
		//TOP Margin - END
		
		//BOTTOM Margin - BEGIN
		
		if (marginLength && doBottomFrame)
		{
			matrix[i].insert(matrix[i].end(), false);//light it up (frame)
		}
		if (doBottomMargin)
		{
			for (uint64_t c = 0; c < bottomMarginWidth; c++)
			{
				matrix[i].insert(matrix[i].end(), true);//dim it (black border)
			}
		}
		//BOTTOM Margin - END

		//TOP and BOTTOM margins - END
	}


	//RIGHT, LEFT margins  - BEGIN

	//Here we're adding additional rows, effectively resulting in Vertical margins in View.

	//LEFT margin - BEGIN
	matrix.insert(matrix.begin(), vMarginOn);

	for (uint64_t i = 0; i < marginLength; i++)
		matrix.insert(matrix.begin(), vMarginOff);
	//LEFT margin - END

	//RIGHT margin - BEGIN

	matrix.insert(matrix.end(), vMarginOn);
	if (marginLength)
	{
		for (uint64_t i = 0; i < marginLength; i++)
			matrix.insert(matrix.end(), vMarginOff);
	}
	}
	//RIGHT margin - END
	
	//RIGHT, LEFT margins  - END

	
	return matrix;
}

/// <summary>
//Returns a QRIntent's QRCode representation rendered as an ASCII string.
//For rendering of QR-Codes within the decentralized terminal we'll be taking
//use of special escape sequence "half block" characters such as.. ▀▄ █
//                                .. dating back to IBM DOS (Unicode 1.1).
/// </summary>
/// <returns></returns>
std::string CQRIntent::getASCIIQRCode(uint64_t& width, std::string newLineCode, const uint64_t& height, bool addSafeMargin, uint64_t terminalWidth)
{
	std::lock_guard<std::recursive_mutex> lock(mGuardian);
	std::string result;
	eColor::eColor borderColor = eColor::cyanOnBlack;
	eColor::eColor dataOnColor = eColor::cyanOnBlack;
	eColor::eColor dataOffColor = eColor::cyanOnBlack;
	size_t additionalSafeMarginSize= addSafeMargin?1:0;
	std::shared_ptr<CTools> tools = CTools::getInstance();

	std::vector<std::vector<bool>> matrix = getQRMatrix(additionalSafeMarginSize,true,true);
	if (matrix.size() == 0)
		return "";
	bool nextLineExists = true;
	width = matrix.size()+ (additionalSafeMarginSize*2);
	std::string horizontalBorderBlock = tools->getColoredString(u8"█", borderColor);
	std::string celingBorderblock = tools->getColoredString(u8"▄", borderColor);
	std::string floorBorderblock = tools->getColoredString(u8"▀", borderColor);

	std::string dataUp = tools->getColoredString(u8"▀", dataOnColor);
	std::string dataDown = tools->getColoredString(u8"▄", dataOnColor);
	std::string dataFull = tools->getColoredString(u8"█", dataOnColor);
	std::string dataOff = tools->getColoredString(u8" ", dataOffColor);

	for (uint64_t y = 0; y < matrix.size() || y< matrix[0].size(); y+=(nextLineExists?2:1))
	{
		if (y == matrix.size() - 1)
			nextLineExists = false;

		//result +=  horizontalBorderBlock;//left border (post-processing)
		for (uint64_t x = 0; x < matrix.size(); x ++)
		{
			if (matrix[x].size() < (y + 2))
				nextLineExists = false;
			else
				nextLineExists = true;

			if (nextLineExists)
			{
				if (matrix[x][y] && matrix[x][y + 1])
					result += dataOff;
				else if (matrix[x][y] && !matrix[x][y + 1])
					result += dataDown;
				else if (!matrix[x][y] && matrix[x][y + 1])
					result += dataUp;
				else result += dataFull;
			}
			else
			{
				 if (matrix[x].size() >= (y + 1))
				{

					if (matrix[x][y])
						result += dataOff;
					else result += dataUp;
				}

			}
		}
		//result += horizontalBorderBlock;//right border (post-processing)


		if (y < matrix.size() - 2)
		result += newLineCode;
	}
	//add some post-processing effects(;])

	//std::string borderTop,borderBottom;
	//for (int i = 0; i < matrix.size() + 2; i++)
	//	borderTop += celingBorderblock;
	//for (int i = 0; i < matrix.size()+2; i++)
	//	borderBottom += floorBorderblock;



	const_cast<uint64_t&>(height) = ((matrix.size() + 2)/2)+1+ (additionalSafeMarginSize*2);
	//std::string readyImage = borderTop + newLineCode +result + borderBottom ;
	std::string readyImage = result ;
	return readyImage;
}


