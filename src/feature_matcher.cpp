#include "feature_matcher.h"
#include "tool.h"

#include <time.h>
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/features2d/features2d.hpp"
//#include "types.hpp"


using namespace cv;
using namespace std;

void Matcher::setFirstFrame(const Mat frame)
{
    if (first_kpt.size() != 0){
        good_matches.clear();
        
        first_kpt.clear();
        first_desc = Mat();
       
    }
    
    if (second_kpt.size() != 0){
        first_match_kpt.clear();
        first_match_desc = Mat();

        
        second_kpt.clear();
        second_match_kpt.clear();
        
        second_desc = Mat();
        second_match_desc = Mat();
    }
    
    first_frame = frame.clone();
    detector->detectAndCompute(first_frame, noArray(), first_kpt, first_desc);

}

void Matcher::findMatch(const Mat frame)
{
    second_frame = frame.clone();
    detector->detectAndCompute(frame, noArray(), second_kpt, second_desc);
    
    vector< vector<DMatch> > matches;
    
    matcher->knnMatch(first_desc, second_desc, matches, 2);

    for(unsigned i = 0; i < matches.size(); i++) {
        if(matches[i][0].distance < nn_match_ratio * matches[i][1].distance) {
            good_matches.push_back( matches[i][0] );
            first_match_kpt.push_back( first_kpt[matches[i][0].queryIdx] );
            second_match_kpt.push_back( second_kpt[matches[i][0].trainIdx] );
        }
    }

	Mat desc_1(good_matches.size(), first_desc.cols, first_desc.type());
    Mat desc_2(good_matches.size(), first_desc.cols, first_desc.type());
	for (unsigned i=0; i<good_matches.size(); i++){
		desc_1.row(i).copyTo( first_desc.row(good_matches[i].queryIdx) );
        desc_2.row(i).copyTo( second_desc.row(good_matches[i].trainIdx) );
	}
    first_match_desc = desc_1.clone();
    second_match_desc = desc_2.clone();

    return;
}


void bruteForceMatching(Mat leftImageGrey, Mat rightImageGrey, vector<Point2f> &k1, vector<Point2f> &k2)
{
    clock_t begin = clock();
    clock_t end = clock();
    double elapsed_sec;
    
    int bestMatchNum = 3;
    
    Mat descriptors_1, descriptors_2;
    std::vector<KeyPoint> keypoints_1, keypoints_2;
//    cv::Ptr<Feature2D> f2d = xfeatures2d::SIFT::create();
//    
//    f2d->detect( leftImageGrey, keypoints_1 );
//    f2d->detect( rightImageGrey, keypoints_2 );
//    
//    f2d->compute( leftImageGrey, keypoints_1, descriptors_1 );
//    f2d->compute( rightImageGrey, keypoints_2, descriptors_2 );
    
    int minHessian = 50;
    

    Ptr<AKAZE> akaze = AKAZE::create();
    akaze->detectAndCompute(leftImageGrey, noArray(), keypoints_1, descriptors_1);
    akaze->detectAndCompute(rightImageGrey, noArray(), keypoints_2, descriptors_2);
    
    end = clock();
    elapsed_sec = double( end - begin ) / CLOCKS_PER_SEC;
    begin = end;
    
    cout << "extract time!!! " <<elapsed_sec <<" s" <<endl;
    
    
    vector< vector<DMatch> > matches;
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce");
    matcher->knnMatch( descriptors_1, descriptors_2, matches, bestMatchNum );
    
//    index_params = dict(algorithm = FLANN_INDEX_KDTREE, trees = 5)
//    search_params = dict(checks = 50)
//    
//    flann = cv2.FlannBasedMatcher(index_params, search_params)
//    
//    matches = flann.knnMatch(des1,des2,k=2)
    
    //look whether the match is inside a defined area of the image
    //only 20% of maximum of possible distance
    //assume rectified image, corresponding features in left and right image are in the same row
    double tresholdDist = 0.20 * leftImageGrey.size().width;
    
    vector< DMatch > good_matches2;
    good_matches2.reserve(matches.size());
    
    end = clock();
    elapsed_sec = double( end - begin ) / CLOCKS_PER_SEC;
    begin = end;
    
    cout << "feature time!!! " <<elapsed_sec <<" s" <<endl;
    
    
    for (size_t i = 0; i < matches.size(); ++i)
    {
        for (int j = 0; j < matches[i].size(); j++)
        {
            Point2f from = keypoints_1[matches[i][j].queryIdx].pt;
            Point2f to = keypoints_2[matches[i][j].trainIdx].pt;
            
            //calculate local distance for each possible match
            double dist = sqrt((from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y));
            
            //save as best match if local distance is in specified area and on same height
            if (dist < tresholdDist && abs(from.y-to.y)<5)
            {
                good_matches2.push_back(matches[i][j]);
                j = matches[i].size();
                
                k1.push_back( from );
                k2.push_back( to );
            }
        }
    }
    
    end = clock();
    elapsed_sec = double( end - begin ) / CLOCKS_PER_SEC;
    begin = end;
    
    cout << "match select time!!! " <<elapsed_sec <<" s" <<endl;
    
//    Mat img_matches;
//    drawMatches( leftImageGrey, keypoints_1, rightImageGrey, keypoints_2, good_matches2, img_matches, Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
//    
//    imshow( "Good Matches", img_matches);
//    waitKey(0);
    return;
}

int stereo_sparse_matching(Mat img1_l, Mat img1_r, Mat img2_l, Mat img2_r, vector<Point2f> &keypoints1_l, vector<Point2f> &keypoints1_r, vector<Point2f> &keypoints2_l, vector<Point2f> &keypoints2_r)
{
    //featureTracking(img1_l, img1_r, img2_l, img2_r, keypoints1_l, keypoints1_r, keypoints2_l, keypoints2_r);
    
    bruteForceMatching( img1_l, img1_r, keypoints1_l, keypoints1_r);
    bruteForceMatching( img2_l, img2_r, keypoints2_l, keypoints2_r);
    
    return 0;
}
