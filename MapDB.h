class MapDB
{
	public:
		struct GPSCoord
		{
			float lon ;
			float lat ;
		};

		struct RegisteredImage
		{
			std::string filename ;		// name of the image in the database directory
			int W,H ;						// width/height of the image
			float scale;					// length of the image in degrees of longitude
			GPSCoord top_left_corner;	// lon/lat of the top-left corner of the image
		};


		MapDB(const std::string& directory_name) ; // initializes the map. Loads the registered images entries from xml file.

	private:
		std::string mRootDirectory ;

		std::vector<RegisteredImage> mImages ;

		GPSCoord mTopLeft ;
		GPSCoord mBottomRight ;
};
