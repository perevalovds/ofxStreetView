//
//  ofxStreetView.cpp
//
//  Created by Patricio Gonzalez Vivo on 3/10/14.
//
//

#include "ofxStreetView.h"

#include "base64.h"
//#include <zlib.h>
#include "Poco/Zip/Decompress.h"
#include "ofxIO.h"

//--------------------------------------------------------------------------------
ofxStreetView::ofxStreetView(){
    clear();
    maxDistance = 200;
    bRegister = false;
    bTexture = true;
    mapWidth = 512;
    mapHeight = 256;
    num_zoom_levels = 3;
    zoom = 3;
}

//--------------------------------------------------------------------------------
ofxStreetView::ofxStreetView(string _pano_id){
    ofxStreetView();
    setPanoId(_pano_id);
}

//--------------------------------------------------------------------------------
ofxStreetView::ofxStreetView(double _lat, double _lon){
    ofxStreetView();
    setLatLon(_lat,_lon);
}

//--------------------------------------------------------------------------------
ofxStreetView::~ofxStreetView(){
    if(bRegister){
        ofUnregisterURLNotification(this);
        bRegister = false;
    }
}

//--------------------------------------------------------------------------------
void ofxStreetView::clear(){
    lat = 0;
    lon = 0;
    bDataLoaded = false;
    bPanoLoaded = false;
    
    pano_id.clear();
    depth_map_base64.clear();
    panoImages.clear();
    links.clear();
    if(panoFbo.isAllocated()){
        panoFbo.begin();
        ofClear(0,0);
        panoFbo.end();
    }
}

//--------------------------------------------------------------------------------
void ofxStreetView::setUseTexture(bool _bUseTex){
    if(_bUseTex&&bDataLoaded&&!bPanoLoaded){
        downloadPanorama();
    }
    bTexture = _bUseTex;
}

//--------------------------------------------------------------------------------
void ofxStreetView::setLatLon(double _lat, double _lon){
    if(!bRegister){
        ofRegisterURLNotification(this);
        bRegister = true;
    }
    
    clear();
    data_url = "http://cbk0.google.com/cbk?output=xml&ll="+ofToString(_lat)+","+ofToString(_lon)+"&dm=1";
    ofLoadURLAsync(data_url);
}

//--------------------------------------------------------------------------------
void ofxStreetView::setPanoId(string _pano_id){
    if(!bRegister){
        ofRegisterURLNotification(this);
        bRegister = true;
    }
    
    if(_pano_id!=pano_id){
        clear();
        pano_id = _pano_id;
        data_url = "http://cbk0.google.com/cbk?output=xml&panoid="+pano_id+"&dm=1";
        ofLoadURLAsync(data_url);
    }
}

/*void ofxStreetView::setZoom(int _zoom){
    zoom = _zoom;
    
	int w = getWidth();
	int h = getHeight();
    panoFbo.allocate(w,h);
    if(zoom>num_zoom_levels){
        zoom = num_zoom_levels;
    }
    
    if (bPanoLoaded) {
        bPanoLoaded = false;
        downloadPanorama();
    }
}*/

//--------------------------------------------------------------------------------
void ofxStreetView::urlResponse(ofHttpResponse & response){
    
    cout << "Request_url: " << response.request.url << endl;
	cout << "    Status: " << response.status << endl;

    if((response.status==200) && (response.request.url == data_url )&& (!bDataLoaded)){
        panoImages.clear();
        
        ofxXmlSettings  XML;
        XML.loadFromBuffer(response.data);

		//cout << "XML ---------: " << response.data << endl;
        
        pano_id = XML.getAttribute("panorama:data_properties", "pano_id", "");
        
        text = XML.getValue("panorama:data_properties:text", "");
        street_range = XML.getValue("panorama:data_properties:street_range",  "");
        region = XML.getValue("panorama:data_properties:region", "");
        country = XML.getValue("panorama:data_properties:country", "");
        
        lat = XML.getAttribute("panorama:data_properties", "original_lat", 0.0);
        lon = XML.getAttribute("panorama:data_properties", "original_lng", 0.0);
        
        elevation = XML.getAttribute("panorama:data_properties", "elevation_wgs84_m", -1);
        
        num_zoom_levels = XML.getAttribute("panorama:data_properties", "num_zoom_levels", 0);
        if(zoom>num_zoom_levels){
            zoom = num_zoom_levels;
        }

		int w_max = XML.getAttribute("panorama:data_properties", "image_width", 1);
		int h_max = XML.getAttribute("panorama:data_properties", "image_height", 1);
		//compute multiplier of zoom level
		int mult = 1;
		int zoom_ = zoom;
		while (zoom_ < num_zoom_levels) {
			zoom_++;
			mult *= 2;
		}
		//compute pano width and height
		pano_width_ = w_max / mult;
		pano_height_ = h_max / mult;

		tile_w = XML.getAttribute("panorama:data_properties", "tile_width", 512);
		tile_h = XML.getAttribute("panorama:data_properties", "tile_height", 512);

		pano_nx_ = pano_width_ / tile_w;
		pano_ny_ = pano_height_ / tile_h;
		if (pano_nx_ * tile_w < pano_width_) pano_nx_++;
		if (pano_ny_ * tile_h < pano_height_) pano_ny_++;
		cout << "pano size: " << pano_width_ << " x " << pano_height_ << ", tile " << tile_w << " x " << tile_h << endl;
		cout << "tiles: " << pano_nx_ << " x " << pano_ny_ << endl;

        
        pano_yaw_deg = XML.getAttribute("panorama:projection_properties", "pano_yaw_deg", 0.0);
        tilt_yaw_deg = XML.getAttribute("panorama:projection_properties", "tilt_yaw_deg", 0.0);
        tilt_pitch_deg = XML.getAttribute("panorama:projection_properties", "tilt_pitch_deg", 0.0);
        
        //Get the base64 encoded data
        depth_map_base64 = XML.getValue("panorama:model:depth_map", "");

        XML.pushTag("panorama");
        XML.pushTag("annotation_properties");
        int nLinks = XML.getNumTags("link");
        for (int i = 0; i < nLinks; i++) {
            Link l;
            l.pano_id = XML.getAttribute("link", "pano_id", "");
            l.yaw_deg = XML.getAttribute("link", "yaw_deg", 0.0f);
            links.push_back(l);
            XML.removeTag("link",0);
        }
        XML.popTag();
        XML.popTag();
        
        //  Decode the depth map ( Credits to Paul Wagener https://github.com/PaulWagener/Streetview-Explorer )
        //  The depth map is encoded as a series of pixels in a 512x256 image. Each pixels refers
        //  to a depthMapPlane which are also encoded in the data. Each depthMapPlane has three elements:
        //  The x,y,z normals and the closest distance the plane has to the origin. This uniquely
        //  identifies the plane in 3d space.
        //
        if(depth_map_base64 != ""){
            //Decode base64
            vector<unsigned char> depth_map_compressed(depth_map_base64.length());
            int compressed_length = decode_base64(&depth_map_compressed[0], &depth_map_base64[0]);
			
			depth_map_compressed.resize(compressed_length);

            //Uncompress data with zlib
            //TODO: decompress in a loop so we can accept any size
            //unsigned long length = 512 * 256 + 5000;
            //vector<unsigned char> depth_map(length);

			//std::string uncompressedString = uncompressed.toString();
			//int zlib_return = uncompress(&depth_map[0], &length, &depth_map_compressed[0], compressed_length);
			//if (zlib_return != Z_OK)	
			//    throw "zlib decompression of the depth map failed";

			
			ofxIO::ByteBuffer depth_map_compressed_buffer(depth_map_compressed);
			ofxIO::ByteBuffer depth_map_uncompressed_buffer;

			std::size_t result = ofxIO::Compression::uncompress(depth_map_compressed_buffer, depth_map_uncompressed_buffer, ofxIO::Compression::ZLIB);
			
			if (result == 0) {
				throw "Error uncompressing depth buffer with ZLIB, empty result";
			}

			const vector<unsigned char> &depth_map = depth_map_uncompressed_buffer.getBuffer();
            

			if (depth_map.size() < 8) {
				throw "Error uncompressing depth buffer with ZLIB, result too short";
			}

            //Load standard data
            const int headersize = depth_map[0];
            const int numPanos = depth_map[1] | (depth_map[2] << 8);
            mapWidth = depth_map[3] | (depth_map[4] << 8);
            mapHeight = depth_map[5] | (depth_map[6] << 8);
            const int panoIndicesOffset = depth_map[7];
            
            if (headersize != 8 || panoIndicesOffset != 8)
                throw "Unexpected depth map header";
            
            //Load depthMapIndices
            depthmapIndices = vector<unsigned char>(mapHeight * mapWidth);
            memcpy(&depthmapIndices[0], &depth_map[panoIndicesOffset], mapHeight * mapWidth);
            
            //Load depthMapPlanes
            depthmapPlanes = vector<DepthMapPlane> (numPanos);
            memcpy(&depthmapPlanes[0], &depth_map[panoIndicesOffset + mapHeight * mapWidth], numPanos * sizeof (struct DepthMapPlane));
        }
        
        bDataLoaded = true;
        
        if(bTexture){
            downloadPanorama();
        }
        
    } else if(response.request.url.find("http://cbk0.google.com/cbk?output=tile&panoid="+pano_id) == 0){
        ofImage img;
		//if response 400, returns black image
		//if (response.status == 200) {
		img.load(response.data);
		//}
        panoImages.push_back(img);
    }
}

//--------------------------------------------------------------------------------
void ofxStreetView::update() {
	if (bDataLoaded && !bPanoLoaded) {

		//Check if time to create pano and depth image

		int n = panoImages.size();
		if (n > pano_nx_ * pano_ny_) {
			ofSystemTextBoxDialog("Loaded too much images for pano: " + ofToString(n));
			OF_EXIT_APP(0);
		}
		if (n == pano_nx_ * pano_ny_) {
			//Create pano and depth map

			//tile size
			int w1 = 1;
			int h1 = 1;
			for (int i = 0; i < n; i++) {
				if (panoImages[i].getWidth() > 0) {
					w1 = panoImages[i].getWidth();
					h1 = panoImages[i].getHeight();
					break;
				}
			}

			//Create fbo and draw on it
			panoFbo.allocate(pano_width_, pano_height_, GL_RGB);
			panoFbo.begin();
			ofClear(0, 0);
			for (int y = 0; y < pano_ny_; y++) {
				for (int x = 0; x < pano_nx_; x++) {
					int i = x + pano_nx_ * y;
					if (panoImages[i].getWidth() > 0) {
						panoImages.at(i).draw(x*w1, y*h1);
					}
				}
			}
			panoFbo.end();

			//all done, delete images
			panoImages.clear();
			bPanoLoaded = true;

			//Create depth
			if (depth_map_base64 != "") {
				makeDepthMesh();
			}

		}

	}
}

//--------------------------------------------------------------------------------
void ofxStreetView::downloadPanorama(){
    if(!bPanoLoaded){
        if(pano_id != ""){
            for(int i = 0; i < pano_ny_; i++){
                for(int j = 0; j < pano_nx_; j++){
                    ofLoadURLAsync("http://cbk0.google.com/cbk?output=tile&panoid="+pano_id+"&zoom="+ofToString(zoom)+"&x="+ofToString(j)+"&y="+ofToString(i));
                }
            }
        }
        
        //if(depth_map_base64 != ""){
        //    makeDepthMesh();
        //}
    }
}

//--------------------------------------------------------------------------------
ofImage ofxStreetView::getDepthMap(){
    ofImage depthImage;
    if(bDataLoaded){
        
        ofPixels depthPixels;
        depthImage.allocate(mapWidth, mapHeight, OF_IMAGE_GRAYSCALE);
        depthPixels.allocate(mapWidth, mapHeight, 1);
        for (int x = 0; x < mapWidth; x++) {
            for(int y = 0; y < mapHeight; y++){
                int planeId = depthmapIndices[y * mapWidth + x];
                if(planeId>0){
                    float rad_azimuth = x / (float) (mapWidth - 1.0f) * TWO_PI;
                    float rad_elevation = y / (float) (mapHeight - 1.0f) * PI;
                    
                    ofPoint pos;
                    pos.x = sin(rad_elevation) * sin(rad_azimuth);
                    pos.y = sin(rad_elevation) * cos(rad_azimuth);
                    pos.z = cos(rad_elevation);
                    
                    DepthMapPlane plane = depthmapPlanes[planeId];
                    float dist = plane.d / (pos.x*plane.x + pos.y*plane.y + pos.z*plane.z);
                    
                    pos *= dist;
                    
                    if(pos.length() > maxDistance){
                        depthPixels.setColor(x, y,ofColor(0));
                    } else {
                        depthPixels.setColor(x, y,ofColor(maxDistance-pos.length()));
                    }
                } else {
                    depthPixels.setColor(x, y,ofColor(0.0));
                }
            }
        }
        depthImage.setFromPixels(depthPixels);
    }
    return depthImage;
}

//--------------------------------------------------------------------------------
void ofxStreetView::makeDepthMesh(){
    meshDepth.clear();
    meshDepth.setMode(OF_PRIMITIVE_TRIANGLES);    
    for (int x = 0; x < mapWidth - 1; x ++) {
        const int next_x = x + 1;
        int map_next_x = next_x;
        if (map_next_x >= mapWidth)
            map_next_x -= mapWidth;
        
        const unsigned int endHeight = mapHeight - 1;
        for (unsigned int y = 0; y < endHeight; y ++) {
            const int next_y = y + 1;
            
            addVertex(next_x, y);
            addVertex(x, y);
            addVertex(x, next_y);
            
            addVertex(x, next_y);
            addVertex(next_x, next_y);
            addVertex(next_x, y);
            
        }
    }
}

//--------------------------------------------------------------------------------
void ofxStreetView::addVertex(int x, int y){
    float rad_azimuth = x / (float) (mapWidth - 1.0f) * TWO_PI;
    float rad_elevation = y / (float) (mapHeight - 1.0f) * PI;
    
    ofPoint pos;
    pos.x = sin(rad_elevation) * sin(rad_azimuth);
    pos.y = sin(rad_elevation) * cos(rad_azimuth);
    pos.z = cos(rad_elevation);
    float distance = 1;
    
    ofPoint normal;
    
    const int map_x = x % mapWidth;
    
    int depthMapIndex = depthmapIndices[y * mapWidth + map_x];
    if (depthMapIndex == 0) {
        distance = maxDistance;
        normal = pos;
        normal.normalize();
    } else {
        DepthMapPlane plane = depthmapPlanes[depthMapIndex];
        distance = -plane.d/(plane.x * pos.x + plane.y * pos.y + -plane.z * pos.z);
        normal.set(plane.x,plane.y,plane.z);
    }
    
    ofVec2f texCoord = ofVec2f((x/(float)mapWidth)*getWidth(),(y/(float)mapHeight)*getHeight());
    pos *= distance;
    
    meshDepth.addNormal(normal);
    meshDepth.addTexCoord(texCoord);
    meshDepth.addVertex(pos);
}

//--------------------------------------------------------------------------------
string ofxStreetView::getCloseLinkTo(float _deg){
    ofPoint direction = ofPoint(cos(_deg*DEG_TO_RAD),sin(_deg*DEG_TO_RAD));
    
    int closer = -1;
    float minDiff = 0;
    for (int i = 0; i < links.size(); i++) {
        
        float angle = links[i].yaw_deg*DEG_TO_RAD;
        ofPoint l = ofPoint(cos(angle),sin(angle));
        
        float diff = l.dot(direction);
        if( diff >= 0 && diff > minDiff){
            minDiff = diff;
            closer = i;
        }
    }
    
    if(closer != -1){
        return links[closer].pano_id;
    } else {
        return "";
    }
}

//--------------------------------------------------------------------------------
float ofxStreetView::getGroundHeight(){
    int groundIndex = depthmapIndices[mapHeight * mapWidth - 1];
    return depthmapPlanes[groundIndex].d;
}

//--------------------------------------------------------------------------------
float ofxStreetView::getWidth(){
    //1.63;//3.26;//6.52;
    //return mapWidth*(1.63*powf(2.0, zoom-1));
	return pano_width_; //512 * pano_nx_;	//TODO hardcoded
}

//--------------------------------------------------------------------------------
float ofxStreetView::getHeight(){
    //return mapHeight*(1.63*powf(2.0, zoom-1));
	return pano_height_; //512 * pano_ny_; //TODO hardcoded
}

//--------------------------------------------------------------------------------
ofTexture& ofxStreetView::getTexture(){
    if(!panoFbo.isAllocated()){
        panoFbo.allocate(getWidth(),getHeight());
    }

    return panoFbo.getTexture();
}

//--------------------------------------------------------------------------------
const ofTexture& ofxStreetView::getTexture() const {
	return getTexture();
}

//--------------------------------------------------------------------------------
ofTexture ofxStreetView::getTextureAt(float _deg, float _amp){
    float widthDeg = getWidth()/360.0;
    
    float offsetX = widthDeg*(pano_yaw_deg-360-_deg+90);
    float amplitud = widthDeg*_amp;
    
    ofFbo roi;
    roi.allocate(widthDeg*_amp, getHeight());
    
    roi.begin();
    ofClear(0,0);
    
    getTexture().draw(-offsetX+amplitud*0.5-getWidth()*2.0,0);
    getTexture().draw(-offsetX+amplitud*0.5-getWidth(),0);
    getTexture().draw(-offsetX+amplitud*0.5,0);
    if(offsetX+amplitud>getWidth()){
        getTexture().draw(-offsetX+amplitud*0.5+getWidth(),0);
    }
    
    roi.end();
    
    return roi.getTexture();
}

//--------------------------------------------------------------------------------
ofPixels &ofxStreetView::getTexturePixels() {
	panoFbo.readToPixels(tex_pixels_);
	return tex_pixels_;
}

//--------------------------------------------------------------------------------
void ofxStreetView::draw(ofPolyRenderMode mode){
    ofPushMatrix();
    //ofRotate(getDirection(), 0, 0, 1);
    getTexture().bind();
    meshDepth.draw(mode);
    getTexture().unbind();
    ofPopMatrix();
}

//--------------------------------------------------------------------------------