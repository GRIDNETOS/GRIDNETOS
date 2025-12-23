#pragma once


/// <summary>
/// 3D point in time.
/// </summary>
class point3D
{
	std::mutex mGuardian;
public:
	void point3D::clear()
	{
		mX = 0;
		mY = 0;
		mZ = 0;
	}


	point3D::point3D()
	{
		mX = 0;
		mY = 0;
		mZ = 0;
		mTime = 0;

	}
	bool point3D::isKnown()
	{		
		std::lock_guard<std::mutex> lock(mGuardian);
		return mX != mY != mZ != 0;
	}

	point3D::point3D(const double& X, const double& Y, const double& Z)
	{
		mX = X;
		mY = Y;
		mZ = Z;
		mTime = std::time(0);
	}
	double point3D::getX()
	{
		std::lock_guard<std::mutex> lock(mGuardian);
		return mX;
	}

	double point3D::getY()
	{
		std::lock_guard<std::mutex> lock(mGuardian);
		return mY;
	}

	double point3D::getZ()
	{
		std::lock_guard<std::mutex> lock(mGuardian);
		return mZ;
	}

	point3D(const point3D& sibling)
	{
		mX = sibling.mX;
		mY = sibling.mY;
		mZ = sibling.mZ;
		mTime = sibling.mTime;
	}

	point3D & operator=(const point3D& sibling)
	{
		mX = sibling.mX;
		mY = sibling.mY;
		mZ = sibling.mZ;
		mTime = sibling.mTime;
		return *this;
	}

	 double point3D::distanceFrom(const point3D &point) const
	{
		//no need for BigFloats, thus we employ hypot() to mitigate overflows.
		 double d = hypot(hypot(mX-point.mX ,mY-point.mY),mZ-point.mZ);
		 return d;
	}
	size_t point3D::getTime()
	{
		return mTime;
	}

	void point3D::ping()
	{
		mTime = CTools::getInstance()->getTime(true);
	}

	double mX;
	double mY;
	double mZ;
	size_t mTime;
};

class CMinecraftPDU
{
private:
	std::mutex mGuardian;
	point3D mPoint;
	size_t mVersion;
	eGridcraftMsgType::eGridcraftMsgType mType;
	std::vector<uint8_t> mPlayerID;
	std::shared_ptr<CTransmissionToken> mTT;
	std::vector<uint8_t> mData;//optional data
public:

	void setPoint(point3D& point);
	bool getPoint(point3D& point);
	CMinecraftPDU();
	CMinecraftPDU(eGridcraftMsgType::eGridcraftMsgType type, point3D point = point3D(),std::shared_ptr<CTransmissionToken> token = nullptr, std::vector<uint8_t> data = std::vector<uint8_t>());
	std::vector<uint8_t> getData();
	std::shared_ptr<CTransmissionToken> getTransmissionToken();
	void initFields();
	void clear();
	size_t getVersion();

	std::string getPlayerID();

	eGridcraftMsgType::eGridcraftMsgType getType();

	static std::shared_ptr<CMinecraftPDU> instantiate(std::vector<uint8_t> data);
	std::vector<uint8_t> getPackedData();
};
