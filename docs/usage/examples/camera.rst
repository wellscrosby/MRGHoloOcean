Visualizing RGBCamera Output
============================

It can be useful to display the output of the RGB camera while an agent is 
training. Below is an example using the ``cv2`` library.

When the window is open, press the ``0`` key to tick the environment and show the
next window.

::

   import holoocean, cv2

   env = holoocean.make("SimpleUnderwater-AUV")
   env.act('auv0', [0,0,0,0,0,0,0,0])

   for _ in range(10):
      state = env.tick()

      pixels = state["RGBCamera"]
      cv2.namedWindow("Camera Output")
      cv2.moveWindow("Camera Output", 500, 500)
      cv2.imshow("Camera Output", pixels[:, :, 0:3])
      cv2.waitKey(0)
      cv2.destroyAllWindows()