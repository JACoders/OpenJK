# Renderer Architecture

The renderer is split into two parts: the front end and the back end.

## Front end
The front end is what cgame and UI modules communicate with. It's responsibility is to build up the scene as it is in the current frame to render as well as the view point from which to render. No GL calls are invoked at any point in this part of the code.

When the scene has been built up, the entities to draw are submitted into a queue to be read by the backend. Entities are sorted on a number of parameters.

## Back end
The back end is responsible for drawing all of the entities submitted to it using OpenGL.
