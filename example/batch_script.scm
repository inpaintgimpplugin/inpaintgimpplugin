(define (simple-inpaint filename_in filename_mask filename_out)
 
   (let* (
	  (image (car (gimp-file-load RUN-NONINTERACTIVE filename_in filename_in)))
          (drawable (car (gimp-image-get-active-layer image)))
	  (new_channel (car (gimp-channel-new image (car (gimp-image-width image)) (car (gimp-image-height image)) "mask" 0 '(255 0 0))))
          (mask (car (gimp-file-load RUN-NONINTERACTIVE filename_mask filename_mask)))
	  (mask_drawable (car (gimp-image-get-active-layer mask)))
          (epsilon 5) 
	  (kappa 25) 
  	  (sigma 1.41) 
	  (rho 4) 
	  (num_channels 4) 
	  (convex (cons-array num_channels 'double))
	 )
   
    ;add mask to image
    (gimp-edit-copy mask_drawable)
    (gimp-image-add-channel image new_channel 0)
    (gimp-edit-paste new_channel TRUE)
    (set! drawable (car (gimp-image-merge-visible-layers image CLIP-TO-IMAGE)))
    
    ;call plugin
    (aset convex 0 33) 
    (aset convex 1 33)
    (aset convex 2 33) 
    (aset convex 3 0)
    (plug-in-inpaint RUN-NONINTERACTIVE
	                   image drawable epsilon kappa sigma rho num_channels convex)
	
    ;save inpainted image                   
    (gimp-file-save RUN-NONINTERACTIVE image drawable filename_out filename_out)
    (gimp-image-delete image)
    (gimp-image-delete mask)
   )
)
    
