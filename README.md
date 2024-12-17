# Mini Minecraft

Team Block Bros: Ivan Zou, Brady Zhou, Zwe Tun
## Video 
[![Minecraft Demo](https://img.youtube.com/vi/0gj0GuymsP0/0.jpg)](https://www.youtube.com/watch?v=0gj0GuymsP0)

## Milestone 1

### Procedural Terrain [Brady]

- How It's Implemented:
  - I first created a file to contain all my noise function implementations. I implemented the well-formed noise (1D & 2D) from lecture, Perlin noise, Voronoi noise, and FBM (which can use a noise function as input). Each of the noises besides the well-formed noise take in a grid-size.
  - In terrian.cpp, I created additional functions use my noise implementations to construct a height map given specific x and z coordinates. Specifically, I had used variations of FBM with Voronoi to generate heights for mountainous terrain and flat terrain. Then, I interpolated if the mountainous terrain was very low. The biome is also set based on this function (will include temperature for additional biomes in the future).
- Difficulties:
  - Originally, I tried using the implementation described in the right up, but I often found that my Perlin noise was returning very small values. When mapped to [0, 1], this resulted in even interpolation between the mountain height and grassland height. This made the grasslands very blocky, and not smooth.
  - I also tried experimenting with different ways to generate terrain, trying to base my approaches off Minecraft's approach (spline points, Perlin noise, etc.). That way, I could just assign a biome to already smooth terrain. However, I struggled to get it to work well, and I eventually abandoned this approach.
- Approach:
  - After experimenting, I found that using Voronoi noise worked a lot better than Perlin noise, so I used it to generate the terrain heights. For the mountain height, this created very nice peaks, but low heights everywhere else (similar to islands, but the islands are mountains). I decided that at these low heights, I can utilize the grassland height function to have actual land. After playing around with different interpolation values, I settled for using a function of the mountain height to interpolate both heights.
  
### Efficient Terrain Rendering and Chunking [Ivan]

- How It's Implemented:
  - Chunk Class:
    - I created a struct to store the VBO data and the indices, another struct to store the position of a Vertex, and another struct to store the direction, direction vector, and vertices on a face of a block
    - Created an updateVBOdata() function that populates the given vbo data and indices vectors with the positions, normals, colors, and indices of the vertices on a face in a given direction to their respective vectors.
    - Created a generateVBOdata() function that populates the struct storing the VBO data by iterating through each block in the chunk, checking if the neighbors of each block is empty, if it is add to the VBO data with the updateVBOdata() function.
    - Implemented the createVBOdata() function by taking the vbo data and indices vectors from the struct, setting their sizes to their corresponding buffer's index counts in Drawable and buffers them into the appropriate VBOs of Drawable.
  - Interleaved Draw Function for ShaderProgram Class:
    - I looked at the original draw() function as a reference, and instead of using individual buffers, I used the interleaved buffer.
    - First, I bind the INTERLEAVED buffer.
    - Since the INTERLEAVED buffer stores vertex data as posnorcol of vec4s repeating, we process the data in intervals of 3 * sizeof(vec4), and each attribute starts at intervals of sizeof(vec4) starting at 0.
    - The rest is the same as the original draw() function.
  - Terrain Class:
    - I updated the draw() function to draw each chunk thats within the given bounds.
    - To expand the terrain, I implemented the expand() function. In the function, I check if there are generated terrain within 2 layers of the player's position, if there isn't then we generate a new 64 x 64 terrain and fill it with new chunks and fill those chunks with a basic floor, leaving it for the Procedural person to change.
  - Updated renderTerrain() in myGL to renders the nine zones of generated terrain that surround the player by getting the x and z of the chunk the player is currently on and drawing a terrain with minX = x - 128 and maxX = x + 128 and the same for z.
- Difficulties:
  - I was confused why getLocalBlockAt() was throwing a out of bounds error, because I thought it did bounds checking with at, but it didn't so I added my own bounds checking to that function, which fixed the issue.
  - When I finished the Chunk class, shader program's interleaved draw, and terrain draw and then ran the program, nothing drew!!! I double, triple, quadruple checked everything, but everything I did seems to make logical sense, so I went to professor's office hours, and he added 2 lines of code to paintGL() and everything drew!
- Approach:
  - I created many structs because it allows me to better organize my code.
  - When expanding the terrain, I decided to create a new terrain and fill it with new chunks when there isn't a generated terrain within 2 layers of terrain from the player's position rather than creating a new chunk and adding it to the map because I think this way makes it easier to create and set a terrain in the generated terrain set, and a generated terrain will contain chunks, so create new chunks where there is no generated terrain to generate the terrain and chunks.
  
### Game Engine Tick Function and Player Physics [Zwe]

- How It's Implemented:
  - The tick function in the game is designed to process game state updates on each frame, everything is based on a delta time. The delta time is calculated by getting the previous stored time and getting the current time by calling QDateTime::currentMSecsSinceEpoch() and finding the difference between the two. By calling tick, player inputs are handle, and physics calculations are handled.
  - Player inputs is where all the acclerations gets set to move our player. By taking in an input bundle we can determine which buttons were pressed exactly. For example if the w key was true then we would set the accleration in the forward direction by a tested positive value. After setting the acclerations the velocities gets sets accordingly where velocity = velocity + accleration. I have implemented a global max and min velocity to enforce a speed limit on our player.
  - The compute physics function is where all the physics calculations are completed. In the beginning we first check for two physics enviroment, flight mode or not flight mode. In flight mode the player is free to fly around the map ignoring gravity and collisions. In non flight mode however the player is subject to gravity and collisions. I implemented gravity as such there is always a present negative accelration down to earth. In order to keep the player from falling through the ground collision detection is on. This collision detection also accounts for the right and forward directions as well so the player cannot go through objects. Finally a friction/air resistence dampening is applied to all velocities so we cannot go forward forever and we loose our momentum over time. 
- Difficulties:
  - The most difficult part was defintely the collision detection. I ran into a lot of issues where I would either get stuck on a block, go through a block or both depending on the orientation. A lot of the experimentation was done early on to conteract some of these issues. For example in order to not get stuck, one idea was to set the velocity slightly back when we collide. However this created jitters around the block as it seeemed like the player is bouncing in and out of the block. Finally figuring out the orientation of the player relative to the object was also difficult as it was important in canceling the velocity in that direction. 
- Approach:
  - My approach to collision detection was to ray cast from all 12 vertices of my player's bounding box. Then ray march on thoes casts to look for intersections. After looping through all 12 vertices' ray march we would find the closest block that is in the way. Afterwards we determine where the block is relative to our player's positon. If the block is in front we must cancel the velocity in forward direction, if the block is to our right we must not be able to go right, etc. Helper functions were utalized in order to account for all cases of collisiosns such as on corners, jumping, moving diagonal, etc.  
  
## Milestone 2

### Cave Systems [Zwe]

- How It's Implemented:
  - To generate caves I utalized a 3D Perlin noise function. This noise function is sample from y values ranging from 0 to the top of the terrrain. To ensure that caves are large enough for players the inputs to the noise function are scaled by values found through numerous tests. 
  -  For the water blocks and lava blocks I needed to first find a way to differentiate between regular ground and thoes specific blocks. In order to do so grid martching is used. This is a continution of the previous's milestone's player physic function. In compute physics we find the block type of the closest blocks to our player in all three axis, x, y and z. After finding the closest block I check if the block is that of water or lava. If that is the case a flag is raise indicating that our player is now standing in these blocks. Using that flag I then turn off collisions only for thoes specific blocks so the player is able to walk through and fall through them. Afterwards I needed to modify the movements so they are slower in thoes blocks. Using the same flags I then change a dampenning factor that is being apply to all the movemnents. So if the flag is raise the dampening factor becomes a less than 1 value that will reduce movement speed, gravity, and turn off the ability to jump. The spacebar gets reconfigured so instead of a positive velocity upwards it becomes an incrementing accelration to simulate the ability to swim.
  -  For the post process filter I pass in a UnifInt for each inWater and inLava flags. A 1 indicates that the player is in thoes enviroment. For the water I wanted to ensure that the blue tint on screen is slighlty more realistic than a blue hue so I used mix to interpolate between a preset blue color and the base color on screen. For the lava a similar method is done but just changing the preset color to that of red. Finally once the flag indicates a 0 the screen is back to normal colors. 
- Difficulties:
  - I had difficulties checking for collisions in water and in lava. In the beginning I was simply checking if the blocks were indicating water however I realized that there were a couple edge cases that led to wrong results. The first case was that if the player stopped moving then there wouldn't be any velocity to grid march along therefore setting the inWater flag as false meaning the player is not in water. However the player could very much be simply standing in water. This would impact the post process filter as well and cause it to flicker from thinking it is out of water to in water whenever the player moves and stop. The next case was if the player was close to the edge of a body of water. This would get the cloesest block as not the water but the blocks surrounding the water which incorrectly raises the wrong flag again. 
- Approach:
  - The solution to the collision problem was two steps. First I needed make sure that I was only checking for water and lava blocks when the player is moving in order to get the correct grid march outputs. To do so I created a boolean playerMoves that is true if any inputs are made from the user. Next I realized that in our mini minecraft water and lava blocks only spawn in discrete and static y ranges. Water is only created from the top of terrains to y = 138 and lava only spawns below y = 25. This meant that for example if I had entereed water I will know for sure I exitted if the player's y goes over 138. Same thing for lava but for y over 25. Using these two steps as a conditional I was able to accurately know when the player is in water and when the player is out of water and same goes for lava. 
  
### Texturing and Texture Animation [Ivan]

- How It's Implemented:
  - To load images as textures in OpenGL in our world, I used the texture class from the lecture recording, and used homework 5's code as a reference to bind the texture file.
  - I created a textures.qrc file and put the path to the textures in that file following the same format as glsl.qrc and I moved the textures file into the assignment_package
  - To separate the VBO data into solid and transparent blocks in the chunk class, I created 2 new vectors in my vboData struct to store the transparent block data and updated my vbo data methods to add to and use the new transparent block vectors accordingly. In order to do this, I also added TRANSPARENT_INDEX and TRANSPARENT_INTERLEAVED buffers to drawable.h, and updated its functions accordingly.
  - To support uv coords in the chunk class, I added uv coords to my Vertex struct and updated my const static array faces to have the uv coords of each vertex on each face of a block in the Vertex struct. Also, I created a mapping of block types to its corresponding uv coord in the texture. Also, in my updateVBOdata(), I added the uv coord of the current block into the vboData vector, and set the third value of the uv vec4 to be its animatable flag.
  - Since vbo data now stores the uv and the texture file or sampler2D contains the alpha value, we don't need col anymore, so I removed it from the chunk class and drawInterleaved().
  - To make ShaderProgram's drawInterleaved() support drawing transparent blocks, I added an isTransparent boolean parameter, such that drawInterleaved() will draw from a certain buffer depending on isTransparent.
  - When drawing chunks, I first drew all opaque/solid blocks first, then drew all transparent blocks afterwards.
  - To animate the water and lava, I created an int animateTime in myGL that increments on every tick and is set to u_Time for the lambert shader. To compute the uv of the animated block, I computed the offset that goes from 0 to a range (2/16) by doing modulo of u_Time * speed, then * range, and after we added the offset to the initial x coord of the uv to get our final uv.
- Difficulties:
  - I had a extremely difficult time trying to connect the texture png to the sampler2D u_Texture. At first, I used the texture class from homework 5, but it was causing "OpenGL error" when I tried running my program. So, I watched the lecture recording about texturing for this milestone, and used the texture class shown in the lecture recordings, but it was still causing the same error. The issue was the texture() function in the glsl file. After a while, I filled the texture() function with a dummy uv value to see if the issue was with my uv value rather than my u_Texture, and when I ran the program, everything drew, so now I know that the issue was with my uv. After looking through my code, I found that I diabled the VertexAttribArray for vs_Col rather than for vs_UV. When I changed it from vs_Col to vs_UV, all my textures successfully drew. Such a tiny bug, caused me such great pain and agony.
  - I had difficulties animating water and lava. At first, when I used u_Time to update the uv, nothing was changing, so I thought u_Time wasn't set correctly, but actually I was just incorrectly updating my uv coords. After thikning about it step by step, I managed to come up with a good way to update the uv with u_Time.
  - There was an issue where the program was crashing on a intel computer, but running fine in a nvidia computer. The problem was in the instanced shader, since it uses a part of the lambert shader, so setting a dummy fs_UV value in the instanced shader fixed the crashing issue.
- Approach:
  - Rather than create a separate vbo to store the uv coords, I stored the uv coords with the position and normal. I did this because this make it so that I have less things to keep track of and manage, and since the vbo data stores vec4s and uv coords are vec2s, I can use the unused parts of the uv vec4 for the animatable flag, which I think is convenient and easier to manage. Also, the uv vec4 will take the place of col, since we no longer need col with uv.

### Multithreaded Terrain Generation [Brady]

- How It's Implemented:
  - I created two thread pools, one for the BlockWorkerThreads and one for the VBOWorkerThreads. Whenever a block needs block data or a chunk needs VBO data, the information is pushed into a shared memory location and the corresponding thread pool wakes up to handle it.
  - The main thread handles all of the OpenGL calls.
- Difficulties:
  - Initially, I was using something similar to the described approach, where I was spawning threads when necessary. However, I found that this was pretty slow so I opted to preinitialize threads to read data from vectors. However, since I did not want each thread to constantly check, I used condition variables to make sure they only wake up when needed.
  - After the multithreading code is able to generate terrain, the program would randomly crash at different times. When using the debugger, it showed that the program was getting a SEGFAULT from accessing a map or an array. However, when checking the values passed in, they were all valid, which was very confusing.
  - After discussing the issue at office hours, it was suggested to switch to using QtRunnable instead. Thus, I refactored my code to do so.
  - To fix the SEGFAULT issues, Professor Mally suggested keeping the chunk instantiation separate from the generation of block data for the chunk. After doing this, I had significantly less crashes.
- Approach:
  - I initialized 2 vectors of 4 threads, one that handles generating block data and one that handles generating VBO data. For these threads, I created two methods. These methods wait on a condition variable within MyGL to ensure the threads are only awake when there is something to process. Once awake, the methods obtains the mutex, reads some amount of data, releases the mutex, and then processes it. It also updates the data queues (once block data is done, the generated Chunk* is passed to vbo generation, etc.).
  - To check for terrain generation, the main thread checks if the current generation zone has changed from the previous one. If it isn't and there is already generated terrain for the current zone (in case of first render), then nothing is done. Otherwise, we destory the VBO data of chunks that are not needed (for GPU space) and check the 20x20 set of chunks surrounding the player. I chose to work with chunks rather than zones because I found it to be easier.
  - When terrain needs to be generated, the (x, z) coordinates are sent to the BlockWorker queue. When already existing data needs VBO data, the Chunk pointer is sent to the VBOWorker queue. Once VBO data is generated, it will be sent to the binding queue, where the main thread binds the VBO data to the GPU using OpenGL.
  - For the QtRunnable version, I created two classes, BlockWorker and VBOWorker, that inherit from QtRunnable. Similar to above, when we need block data, we spawn a BlockWorker thread. The VBOWorker does the same thing as before and generates VBO data per Chunk.
  
## Milestone 3

### New Biomes [Brady]

- How It's Implemented:
  - I used two noise maps for moisture and temperature. These values determined which biome would be at a particular x and z position. To determine height, each biome generates its own height value. We then use bilinear interpolation to interpolate between the 4 possible biome heights.
  - We also defined a new height function for the SNOWY PLAINS biome.
  - For these new biomes, we need to define additional block types. For desert, we define SAND. For the SNOWY PLAINS, we define SNOW and ICE. In the SNOWY PLAIN, instead of putting water, we use ICE.
- Difficulties:
  - Initially, instead of a SNOWY PLAINS biome, I wanted to make an ISLAND biome. For this biome, it would be mostly water with some occasional land, where the lower blocks were SAND and the higher blocks were GRASS. When testing the biome alone, it looked fine. However, once we included the interpolation, the other biome heights tended to affect the island biome too much. This resulted in very bad looking islands, and it was difficult to tell what biome it actually was.
  - Another issue was that when using moisture and temperature directly, the terrain height did not reflect the biomes very well. For example, the mountain biome ended up have very few peaks, and the desert biome's height was pretty high.
- Approach:
  - For the noise maps, I used perlin noise with smoothstep for moisture and voronoi noise for temperature.
  - For bilinear interpolation, I interpolated using moisture first, followed by temperature. Since desert and grassland have similar heights and looks, I used the same height map for both, except I added additional base height to the desert to make it slightly taller. To make the biomes look better, I smoothstepped moisture and temperature in the interpolation so that the actual biome height is shown until the biome is about to change.

### Day and Night Cycle, Distance Fog, Water Waves [Ivan]

- How It's Implemented:
  - Water Waves:
    - In the Vertex Shader, I distorted the surface of the water block with 3 different waves with different amplitudes, frequencies, speeds and adding them to the y of the model position, then I updated the normals of the x and z accordingly using the gradient of the wave equation.
    - In the Fragment Shader, I added a uniform u_CamPos, which is just the position of the player, and then I computed the specular term using the Blinn-Phong shading model, which I add to lightIntensity.
  - Distance Fog:
    - In the Fragment Shader, I computed the distance the blocks are away from the camera. Then, I initialized two floats, fogStart and fogEnd, where fogStart is the number of blocks away from the camera the fog will start and fogEnd is the number of blocks away from the camera the fog will become fully opague. After, I computed a fog factor using the block distance and the fog start and end. Then, I interpolated the shaded color with a constant fog color by the fog factor.
  - Day and Night Cycle:
    - I created a Quad class that inherits from Drawable, so that we can draw a sky. In the Quad Class, I hardcoded the vertex positions and uvs for the vbo data.
    - I created sky vert and frag glsl files.
    - In MyGL, I initialized a Quad object and a ShaderProgram for the sky glsl files. Then, in initializeGL(), I created the quad's vbo data and createdd the sky ShaderProgram with the sky vert and frag glsl files.
    - I updated ShaderProgram's draw() function to support vs_UV. Then, I drew the quad in MyGL's paintGL() function with the sky ShaderProgram.
    - In the sky fragment shader, I initially followed the sky lecture recording to create a static sunset scene as a baseline. After, I made the sun direction change based on u_Time, and I made the sky color transition from a setting color and a base daytime and nighttime color based on the sun's elevation.
    - I updated the light direction in the terrain by using the exact same computation to change the sun's direction in the sky based on u_Time, and then setting the light direction to the sun's direction.
    - I updated the fog color in the terrain by using the exact same method of computing the sky color, and then setting the sky color to the fog color.
    - I drew the moon by calculating its direction as opposite to the sun's direction, which makes it only appear at night. Then, I use the dot product between the ray direction and the moon's direction to calculate its visibility, and creating a circular glow and soft edges with smoothstep, and mixing it with the finalSkyCol to add the moon to the sky.
- Difficulties:
  - For water waves, I had difficulty updating the normals, but after experimenting with different formulas, I found an acceptable way to update the normals for lighting.
  - For Day and Night Cycle, I had trouble smoothly transitioning from daytime to sunset to nighttime and then to sun rise. When I tried to transition between those four using smoothstep() and mix() only, only either daytime or nighttime will show up with the sun set and sun rise. The solution is to use an if statement and mix() depending on the sun's elevation, and by testing out different intervals to smoothstep to find the best one.
  - When I was trying to update the fog color to be the sky color, I was confused why the fog color wasn't changing after my changes. It was because I set the w component of the vec4 fogColor to 0, when converting from vec3 to vec4. After, setting the w component to 1, the fog color changed.
- Approach:
  - For distance fog, I made the fog start after a certain number of blocks because that how minecraft does it.
  - For Day and Night Cycle, I made each cycle is 24 minutes because that's how it is in Minecraft. 

### Post Process Overlay, Procedurally Placed Assets, Procedural Grass Colors [Zwe]

- How It's Implemented:
  - I implemented post process overlay as a continution of my milestone 2 tint. In the fragement shader I needed two time varying effects. One for water a bubbly effect and one for the lava a fire like lava flow. In order to create the bubbily effect I utalized Worley Noise and varied the points through time. For the lava effect I first distorted the UV through FBM then applied the same Worley. This created this spider web like of lava flow effect perfect for when player falls into lava.
  - For the proceduarlly placed assets, I made biome specific assets first. For a default tree asset I got inspiration from the following video to use for loops to stack leaves in the shape of minecraft trees: https://www.youtube.com/watch?v=GdHtYQuDOMM. To place these trees I utalized FBM as the noise function to evenly and dispersly placed tree. For each biome I made different assets, for example deserts have cactuses, snowy planes have dead trees and snow covered trees. For grasslands there are also fallen logs.
  - For procedural grass colors I first needed to procedurally place the grass patches across the terrain and biomes. To make patches I utalized 2D perlin noise because just like the caves we want chambers like areas to simulate the nature of grass patches. Then in the fragment shader I have uv flags to know which are grass patches to gradient them. For grasslands my grass patches are to simulate dead grass patches by using yellow grass then slowly transitioning in coloring to the regular green grass. We can get the orginal color of each biome's terrain block by navigating the texture map from the current uv of the patch. To create the gradient I decided to create a float falloff that depends on the center of the patch and the current position to accuratly mix the colors. I repeat this process for each biome's patches. For snowy and mountain biomes it is grassy snow patch where grass transitions to full snow. For desert it is sand crack patches where the sand cracks transition into regular sand. Finally as mentioned before the grasslands had dead grass patches where yellow grass transitions to green grass. This overall makes the general world terrain more natural looking as there are less uniform colors. 
- Difficulties:
  - For post process I had trouble with binding the framebuffer. When I binded it, it would zoom in on a specific part of the screen.
  - For procedural asset I had trouble creating cactuses. Becuase the cactus texture do not take up the entire block the sides of it where emppty but because the program thinks it is still a full block size the background of the cactus is not rendered. This left the cactus to show the background sky on its side. 
  - For the procedural grass colors I had difficulty on the gradient in order to make it smoothly transiiton from the patch color to the normal colors of the biome. 
- Approach:
  - To fix the framebuffer, in office hours Adam removed the scaling by the device pixel ratio in render3Dscene.
  - To fix the cactus, I was told by a TA to update the VBO data of the background blocks when there is a cactus. So whenever the neighbor blocks being created was a cactus block I now know I needed to still update its VBO data. So in my chunk class in generateVBO I was able to change the if statement to include a check to see if the neighbor block was a cactus and if so I still update its VBO data to be drawn. 
  - For the procedural grass a grid based center position was used to make the fall off. This was remeniscent of my pixelate function from the post process homework where I used a grid based uv corrdinate to sample the color.
