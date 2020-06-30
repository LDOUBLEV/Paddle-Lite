//
// Created by liuweiwei06 on 2020-06-23.
//

#include <iostream>
#include <vector>
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "math.h"

#include <iostream>
#include <cstring>
#include <fstream>

#define character_type "ch"
#define max_dict_length 6624
const std::vector<int> rec_image_shape {3, 32, 320};


cv::Mat crnn_resize_norm_img(cv::Mat img, float wh_ratio){
  int imgC, imgH, imgW;
  imgC = rec_image_shape[0];
  imgW = rec_image_shape[2];
  imgH = rec_image_shape[1];

  if (character_type=="ch")
    imgW = int(32*wh_ratio);

  float ratio = float(img.cols)/float(img.rows);
  int resize_w, resize_h;
  if (ceilf(imgH*ratio)>imgW)
    resize_w = imgW;
  else
    resize_w = int(ceilf(imgH*ratio));
  cv::Mat resize_img;
  cv::resize(img, resize_img, cv::Size(resize_w, imgH),0.f, 0.f, cv::INTER_CUBIC);

  resize_img.convertTo(resize_img, CV_32FC3, 1 / 255.f);

  for (int h=0; h< resize_img.rows; h++){
    for (int w=0; w< resize_img.cols; w++){
      resize_img.at<cv::Vec3f>(h, w)[0] = (resize_img.at<cv::Vec3f>(h, w)[0] - 0.5) *2;
      resize_img.at<cv::Vec3f>(h, w)[1] = (resize_img.at<cv::Vec3f>(h, w)[1] - 0.5) *2;
      resize_img.at<cv::Vec3f>(h, w)[2] = (resize_img.at<cv::Vec3f>(h, w)[2] - 0.5) *2;
    }
  }

  cv::Mat dist;
  cv::copyMakeBorder(resize_img, dist, 0, 0, 0, int(imgW-resize_w), cv::BORDER_CONSTANT, {0, 0, 0});

  return dist;

}

cv::Mat crnn_resize_img(cv::Mat img, float wh_ratio){
  int imgC, imgH, imgW;
  imgC = rec_image_shape[0];
  imgW = rec_image_shape[2];
  imgH = rec_image_shape[1];

  if (character_type=="ch")
    imgW = int(32*wh_ratio);

  float ratio = float(img.cols)/float(img.rows);
  int resize_w, resize_h;
  if (ceilf(imgH*ratio)>imgW)
    resize_w = imgW;
  else
    resize_w = int(ceilf(imgH*ratio));
  cv::Mat resize_img;
  cv::resize(img, resize_img, cv::Size(resize_w, imgH),0.f, 0.f, cv::INTER_LINEAR);

  return resize_img;
}

std::basic_string<char, std::char_traits<char>, std::allocator<char>> * read_dict(std::string path){

  std::ifstream ifs;
  std::string charactors[max_dict_length];

  ifs.open(path);
  if (!ifs.is_open())
  {
    std::cout<<"open file "<<path<<"	failed"<<std::endl;
  }
  else
  {
    std::string con = "";
    int count = 0;
    while (ifs)
    {
      getline(ifs, charactors[count]);
      count++;
    }
  }
  return charactors;
}

cv::Mat get_rotate_crop_image(cv::Mat srcimage, std::vector<std::vector<int>> box){
  cv::Mat image;
  srcimage.copyTo(image);
  std::vector<std::vector<int>> points = box;

  int x_collect[4] = {box[0][0], box[1][0], box[2][0], box[3][0]};
  int y_collect[4] = {box[0][1], box[1][1], box[2][1], box[3][1]};
  int left = int(*std::min_element(x_collect, x_collect+4));
  int right = int(*std::max_element(x_collect, x_collect+4));
  int top = int(*std::min_element(y_collect, y_collect+4));
  int bottom = int(*std::max_element(y_collect, y_collect+4));

  cv::Mat img_crop;
  image(cv::Rect(left, top, right-left, bottom-top)).copyTo(img_crop);

  for(int i=0; i<points.size(); i++){
    points[i][0] -= left;
    points[i][1] -= top;
  }

  int img_crop_width = int(sqrt(pow(points[0][0] - points[1][0], 2) +
                                pow(points[0][1] - points[1][1], 2)));
  int img_crop_height = int(sqrt(pow(points[0][0] - points[3][0], 2) +
                                 pow(points[0][1] - points[3][1], 2)));

  cv::Point2f pts_std[4];
  pts_std[0] = cv::Point2f(0., 0.);
  pts_std[1] = cv::Point2f(img_crop_width, 0.);
  pts_std[2] = cv::Point2f(img_crop_width, img_crop_height);
  pts_std[3] = cv::Point2f(0.f, img_crop_height);

  cv::Point2f pointsf[4];
  pointsf[0] = cv::Point2f(points[0][0], points[0][1]);
  pointsf[1] = cv::Point2f(points[1][0], points[1][1]);
  pointsf[2] = cv::Point2f(points[2][0], points[2][1]);
  pointsf[3] = cv::Point2f(points[3][0], points[3][1]);

  cv::Mat M = cv::getPerspectiveTransform(pointsf, pts_std);

  cv::Mat dst_img;
  cv::warpPerspective(img_crop, dst_img, M, cv::Size(img_crop_width, img_crop_height), cv::BORDER_REPLICATE);

  if (float(dst_img.rows) >= float(dst_img.cols)*1.5){
    cv::Mat srcCopy = cv::Mat(dst_img.rows, dst_img.cols, dst_img.depth());
    cv::transpose(dst_img, srcCopy);
    cv::flip(srcCopy, srcCopy, 0);
    return srcCopy;
  }
  else{
    return dst_img;
  }

}

template<class ForwardIterator>
inline size_t argmax(ForwardIterator first, ForwardIterator last)
{
  return std::distance(first, std::max_element(first, last));
}