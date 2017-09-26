## FrameScale
Convert pixel format **NV12** -> **RGB24**, so that output is compatible with OpenGL API. 
Test Input is 4K 903frames NV12 raw video(9.29 GB).
## Pixel Format 
common pixel format comparison.  
Instances of yuv420 category present distinction in UV order. 
#### yuv422
* YUVY ![MpvDO.png](https://s1.ax1x.com/2017/09/20/MpvDO.png "yuvy")
* UYVY ![M9pUH.png](https://s1.ax1x.com/2017/09/20/M9pUH.png "uyvy")
* YUV422P ![MpxbD.png](https://s1.ax1x.com/2017/09/20/MpxbD.png "yuv422p")
#### yuv420
* YV12,YU12 ![M995d.png](https://s1.ax1x.com/2017/09/20/M995d.png "yv12yu12")
* NV12,NV21 ![M9SVe.png](https://s1.ax1x.com/2017/09/20/M9SVe.png "nv12nv21")

>**Note**  
>I420: YYYYYYYY UU VV    =>YUV420P  
>YV12: YYYYYYYY VV UU    =>YUV420P  
>NV12: YYYYYYYY UVUV     =>YUV420SP  
>NV21: YYYYYYYY VUVU     =>YUV420SP  
## Result
Algorithm | Speed(sec) | Algorithm | Speed(sec)
--- | :---:|--- | :---:
SWS_BILINEAR | 22|SWS_DIRECT_BGR | 37
SWS_FAST_BILINEAR | 21|SWS_DIRECT_BGR | 37
SWS_AREA | 22|SWS_GAUSS | 37
SWS_BICUBIC | 37|SWS_POINT | 22
SWS_BICUBLIN | 23|SWS_X | 23
SWS_DIRECT_BGR | 37
## Reference

[azraelly - cnblogs](http://www.cnblogs.com/azraelly/archive/2013/01/01/2841269.h)   
[pixel - wikipedia](https://en.wikipedia.org/wiki/Pixel)   
[yuv - wikipedia](https://en.wikipedia.org/wiki/YUV) 
