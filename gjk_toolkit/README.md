<h1>GJK_TOOLKIT</h1>

Collision detection is pivotal to game development, and doing so quickly and accurately will help to create a stronger gaming experience.  One of my favorite collision detection algorithms, the __Gibert-Johnson-Keerthi (GJK)__ algorithm can solve for collisions between any two convex polygons of any size very quickly.  To do this, it uses the __Minkowski Sum__ (or Difference, depending on whom you ask) and then does simple converging spline-checking to detect a collision.

<h2>What am I seeing?</h2>

The Red and Blue squares are the player and wall, respectively.  We can move the player around the scene, and then check for a collision.

The Green dots are the Minkowski Difference of the two shapes.

The single White dot is the origin.

<h2>Controls:</h2>

"WASD" to Move "player" around while in Normal mode.

"U" Toggles between Normal and GJK Mode.  Normal mode has no bars on the side, and allows you to move the player around.  GJK mode freezes player movement (and hides the player and wall) and allows you to run the GJK algorithm.  GJK mode will have colored bars on the sides.

"I" IF Frozen (GJK Mode), this run GJK and highlights the side bars Green (NO COLLISION) or Red (COLLISION).  The Simplex dots are rendered in Purple.

"Q" Quits the GJK_TOOLKIT.