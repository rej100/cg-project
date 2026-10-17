# 3DCGA Project

## Scene plan

### Concept: Reactor Inspection

A bright, clean energy exhibit in an open-air pavilion. A white-and-gold reactor
stands at the center of a white marble circular platform, while a small
inspection drone flies around it. The visual direction is a premium technology
showroom: simple shapes, polished marble with subtle grey veins, satin ceramic,
gold trim, a bright sky, and restrained cyan accents. Manufactured panels,
grooves, and markings add detail to the reactor and drone.

The scene runs automatically in one application. Camera presets and independent
UI controls let us inspect each graphics technique, pause the scene, and capture
matching before/after screenshots. The following is the planned scope, rather
than a list of features already implemented.

### Layout and animation

- **Exhibit platform:** approximately 12 m across, raised slightly above a broad,
  pale ground plane. Its polished white marble top has restrained grey veining
  and a narrow gold outer edge. It receives shadows from the reactor and drone.
- **Reactor:** approximately 3.5 m tall, with a white marble cylindrical pedestal
  and gold collar, a luminous cyan core, a dark inner recess, white ceramic
  housing panels, three gold rings, and three structural supports. One
  outward-facing panel prominently displays a blue TU Delft logo with a shallow,
  beveled, embossed appearance produced by its normal map, alongside grooves and
  screw heads. Keep the area around the logo uncluttered and frame it clearly in
  the showcase and material close-up views. The pedestal provides a close-up of
  marble beside gold.
- **Inspection drone:** approximately 1.2 m wide, with a white body, gold trim,
  four rotor assemblies, and a dark articulated sensor. It follows a closed loop
  around the reactor, approximately 3–4.5 m from its center, with gentle changes
  in height. Its blades spin, its sensor mount turns, and its sensor head tilts.
- **Showcase lamp:** a fixed, approximately 6 m tall mast beside the platform,
  with an angled overhead arm and a luminous lamp face. Its spotlight points
  across the reactor and intersects part of the drone route.
- **Environment:** a bright daylight sky with distant features that give metal
  reflections recognizable structure. The same environment supplies the visible
  background and environment reflections.
- **Lighting:** a stationary directional sun and a stationary showcase spotlight
  illuminate the scene from different angles. Both cast shadows. Balance them
  so the spotlight makes a visible contribution while the overall scene stays
  bright.
- **Energy effects:** a small stream of cyan particles rises through and just
  above the reactor core. The dark inner recess provides contrast. Bloom gives
  the core, lamp face, and selected particles a restrained halo.

The drone route has four cubic Bézier segments with smooth joins, including the
closing join. Its heading follows the route. A chosen pause position places the
drone inside the spotlight beam with both light-source shadows visible on the
platform.

### Scene inventory

#### Models and geometry

Create the scene models in Blender or obtain suitable models online, then use
Blender to prepare their geometry, materials, scale, and UVs. UVs describe how 2D
texture images map onto each model's surface. Export meshes as OBJ files and
textures as separate PNG images for the application to load.

The drone's individual parts are also authored assets. Its parent–child hierarchy,
local transforms, and animation are constructed in application code to satisfy
the hierarchical-transformation requirement. Export its moving parts separately
with appropriate rotation pivots.

| Object | Required parts | Construction / animation |
| --- | --- | --- |
| Ground | One broad ground plane | Static plane with a uniform pale material. |
| Exhibit platform | Circular base, marble top surface, narrow gold edge | Authored static mesh with small edge bevels; marble top and gold edge have separate material assignments. |
| Reactor pedestal | Marble base cylinder and gold collar | Part of the authored reactor asset; shares the platform's marble texture set at a consistent world-space scale. |
| Reactor core | Cyan inner cylinder and dark backing/recess | Part of the authored reactor asset; core has an emissive material. |
| Reactor housing | Three white outer panels and one visible detail panel bearing the TU Delft logo | Authored panels with readable UVs for the colored logo, its normal-mapped bevels, grooves, and markings. |
| Reactor structure | Three gold rings and three supports | Static parts included in the reactor asset, with beveled exposed edges. |
| Drone body | Main shell, trim, four rotor mounts, and fixed hubs | Authored body mesh; its root transform follows the path and is inherited by the other parts through a hierarchy built in code. |
| Drone rotors | Four two-blade assemblies | One authored blade-assembly mesh reused four times; code spins each instance around its mount. Blade shape makes rotation visible. |
| Drone sensor | Turning mount, tilting head, and dark lens | Separate mount and head meshes; code constructs body → turning mount → tilting head, with the lens included in the head mesh. |
| Showcase lamp | Mast, overhead arm, lamp housing, luminous face | Authored static mesh; the spotlight created in code matches the fixture's position and direction. |
| Sky background | Camera-centered cube | Small code-generated rendering helper that samples the environment cubemap. |
| Energy particles | Reusable camera-facing quad | Code-generated rendering helper; particle instances have position, velocity, age, size, and opacity. |
| Path visualization | Sampled line along the four Bézier segments | Generated from the actual flight path; UI-controlled visibility. |

The ground plane, skybox cube, particle quads, and path lines are simple rendering
helpers generated in code. The exhibit and drone geometry come from asset files.

#### Model exports and editable source files

Keep editable originals in `assets/source/`, exported meshes in `assets/models/`,
and exported textures in `assets/textures/`. Use one Blender file per major asset
so the two group members can work on different assets independently. Keep source
files and exports in version control; the application loads the exported meshes
and images without requiring Blender at runtime.

| Planned file | Contents / use |
| --- | --- |
| `assets/source/platform.blend` | Editable platform geometry, UVs, marble/gold material setup, and texture-baking setup. |
| `assets/source/reactor.blend` | Editable reactor, pedestal, and detail panel, including the detailed logo geometry used for baking. |
| `assets/source/drone.blend` | Editable body, rotor, sensor mount, and sensor head meshes, with reference positions and pivots for assembly. |
| `assets/source/showcase_lamp.blend` | Editable lamp geometry and material assignments. |
| `assets/source/tu_delft_logo.svg` | Vector source for the logo artwork and beveled relief; source and authorship recorded in `assets/README.md`. |
| `assets/models/platform.obj` | Static platform, with material groups for its marble top, base, and gold edge. |
| `assets/models/reactor.obj` | Static reactor assembly: pedestal, collar, core, recess, panels, rings, and supports, with named material groups. |
| `assets/models/showcase_lamp.obj` | Static fixture with separate housing and emissive-face material groups. |
| `assets/models/drone_body.obj` | Shell, trim, fixed rotor mounts, and hubs. |
| `assets/models/drone_rotor.obj` | One two-blade assembly centered on its rotation pivot, instantiated four times. |
| `assets/models/drone_sensor_mount.obj` | Turning sensor mount centered on its yaw pivot. |
| `assets/models/drone_sensor_head.obj` | Tilting sensor head and lens, centered on its tilt pivot. |

OBJ files contain mesh coordinates rather than Blender's object origins or
parenting. Export each moving drone part with its pivot at `(0, 0, 0)` in its mesh
coordinates, and define its assembly offset relative to its parent in code.

Retain matching `.mtl` companions produced by OBJ export: `platform.mtl`,
`reactor.mtl`, `showcase_lamp.mtl`, `drone_body.mtl`, `drone_rotor.mtl`,
`drone_sensor_mount.mtl`, and `drone_sensor_head.mtl`, alongside their OBJ files.
Use material names to assign our PBR parameters and texture sets in the
application; Blender shader-node graphs are baked into images rather than
reconstructed from the MTL files. Record online asset sources, authors, licenses,
and any modifications in `assets/README.md` for attribution in the report.

#### Texture and environment files

These are planned paths for assets to create or obtain. Each texture set is
shared by the relevant object's parts; gold, aluminium, the sensor lens, and
emissive surfaces can use constant material parameters instead of extra images.

| Planned file | Purpose |
| --- | --- |
| `assets/textures/marble_basecolor.png` | Polished white marble with subtle grey veins, shared by the platform top and reactor pedestal. |
| `assets/textures/marble_roughness.png` | Low roughness with subtle natural variation across the marble. |
| `assets/textures/marble_normal.png` (optional) | Very fine surface detail; veins remain color patterns rather than deep grooves. A flat normal is sufficient initially. |
| `assets/textures/reactor_basecolor.png` | White housing panels, the blue TU Delft logo, and diagnostic labels. |
| `assets/textures/reactor_roughness.png` | Satin housing surfaces and smoother selected details. |
| `assets/textures/reactor_normal.png` | Shallow beveled relief for the TU Delft logo, recessed grooves, and screw heads; especially clear on the detail panel. |
| `assets/textures/drone_basecolor.png` | White shell, colored identification markings, and small labels. |
| `assets/textures/drone_roughness.png` | Surface-finish variation across the shell and trim regions. |
| `assets/textures/drone_normal.png` | Panel seams and small manufactured details. |
| `assets/textures/particle_soft.png` | Soft particle sprite with an alpha falloff; tinted cyan during rendering. |
| `assets/environment/pavilion_daylight.hdr` | Bright panoramic environment, converted to a cubemap for the background and reflections. |
| `assets/README.md` | Asset inventory, authorship, source links, licenses, and modifications, including our own generated assets. |

Base-color textures use sRGB color interpretation; roughness and normal maps are
linear data. Normal maps use a consistent tangent-space convention. Marble,
ceramic, gold, aluminium, and dark sensor materials have separate
metallic/roughness defaults so they remain distinct when texture sampling is
disabled. Marble is a nonmetallic, opaque PBR material; its polished finish
contrasts with the satin ceramic housing. Gold trim uses separate geometry and a
metallic material, keeping the marble texture set simple.

For the TU Delft logo, use the vector outline to create shallow raised geometry
with beveled edges in Blender. Bake the detailed panel's tangent-space normals
onto the simple, UV-mapped runtime panel and combine its other details into the
same reactor normal map. Align the blue artwork in the base-color texture with
that relief. The runtime logo's apparent depth comes from normal mapping on the
simple panel, leaving its silhouette unchanged. With normal mapping disabled,
the blue logo remains visible as a flat printed design.

#### Asset preparation and first milestone

1. **Validate one asset end to end first:** create a small marble pedestal with a
   gold collar and a TU Delft detail panel in Blender. Use `reactor.blend` and the
   reactor export paths for this initial version, then expand it into the full
   reactor after the pipeline works.
2. **Prepare UVs and materials:** unwrap the model so the color, roughness, and
   normal textures align. Keep the logo readable and marble vein scale consistent
   between surfaces. Author materials or adapt suitable online texture sets.
3. **Bake textures:** export base color without lighting or shadows, roughness as
   linear data, and normal maps in tangent space. Bake procedural Blender
   materials into images using the planned texture paths.
4. **Export meshes:** use meters and a consistent Y-up axis convention, apply
   object rotation/scale, and center moving-part mesh coordinates on their pivots.
   Export triangulated geometry with normals, UVs, and named material groups.
   Bake normal maps against the same triangulated mesh and shading normals that
   will be exported.
5. **Verify in the application:** load the OBJ and texture files, generate the
   tangent basis from the mesh normals/UVs using conventions matching the bake,
   and check scale, orientation, material assignments, pivots, and normal-map
   direction. Confirm the logo's normal-map toggle works and that the marble and
   gold read as distinct materials.
6. **Build out the inventory:** use the verified conventions for the full reactor,
   platform, lamp, and separate drone parts. Keep editable Blender sources and
   exports synchronized as assets change.

#### Other required scene data and rendering resources

- **Asset loading and scene data:** load the model and texture files, then define
  object placement, material assignments, light settings, four Bézier segments,
  and camera presets in code. Construct the drone's parent–child relationships,
  pivot offsets, and component animation in code from its separate mesh assets.
- **Camera presets:** whole-scene showcase, overhead route view, drone-follow
  view, reactor material close-up, and drone mechanism close-up.
- **Material rendering:** PBR shading, an independently selectable simple
  reference shader mode, material-texture sampling, tangent-space normal mapping,
  and environment reflection sampling.
- **Environment preprocessing:** runtime conversion of the HDR panorama to a
  cubemap and roughness filtering for reflections.
- **Shadow rendering:** one depth map per light, with an orthographic projection
  for the sun and a perspective projection for the spotlight. Update both while
  the drone moves; share the depth-rendering code between the lights.
- **Effect rendering:** particle billboards, a path-line renderer, and an HDR
  scene target followed by bright-region extraction, blur, and bloom composition.
- **Runtime-generated data:** rendering-helper meshes, tangent data for imported
  meshes, path samples, the cumulative distance lookup table for constant-speed
  motion, particles, environment maps, shadow maps, and post-processing buffers.
  These are generated resources rather than additional asset files to download or
  commit.

### Planned mandatory features

| Feature | Where it appears | How we show it / UI controls |
| --- | --- | --- |
| Multiple viewpoints | The platform, flight route, drone, and reactor details. | Switch between showcase, overhead, follow, and close-up camera presets while the same scene runs. |
| Advanced shading: PBR | Polished marble beside gold trim, white ceramic, gold reactor rings, aluminium supports, and the dark sensor. Curved surfaces expose highlight shape and material differences. | Compare PBR against the simple reference shading mode at a fixed camera, light setup, and exposure. Include the marble pedestal and gold collar in a material close-up. |
| Material textures | Marble veining on the platform and pedestal, reactor labels, drone markings, and spatial roughness variation. | Toggle material textures against constant material values. Marble veins and printed markings disappear, and roughness becomes uniform per material. Keep normal mapping separately controllable. |
| Normal mapping | Embossed TU Delft logo, grooves, and screw heads on the reactor detail panel, plus drone panel detail. | In the reactor close-up, toggle normal mapping while keeping material textures enabled. The logo changes from apparently raised and beveled to flat printed blue artwork; grooves and screw heads flatten too, while the mesh silhouette stays unchanged. |
| Environment mapping | Gold reactor rings and metallic drone trim reflect the sky and distant environment. | Move the camera around the curved metal and toggle its environment-reflection contribution. Keep the background and direct lighting visible. |
| Smooth paths using at least three cubic Bézier curves | The drone flies a closed route comprising four smoothly joined cubic segments. | Toggle path movement and the rendered curve independently. The overhead view exposes the route; the showcase view exposes height changes. |
| Hierarchical transformations | Code-assembled drone body → rotor mounts → spinning blades, and body → turning sensor mount → tilting sensor head, using separately loaded mesh assets. | Pause route movement while component animation continues, then toggle component animation separately. Use the drone close-up to show the nested motion. |

### Planned extra features

| Feature | Where it appears | How we show it / UI controls |
| --- | --- | --- |
| Constant-speed Bézier movement | The drone maintains approximately constant world-space speed across the complete route using a sampled cumulative-distance lookup table. | Switch between uniform parameter stepping and constant-distance stepping, with the same overall lap duration. Show a speed readout and use a route with visibly different segment shapes. |
| Shadows from multiple light sources | Both the sun and showcase spotlight cast reactor and drone shadows onto nearby surfaces. Different light directions separate their shadows. | Provide independent light and shadow toggles for each source. Pause the drone in the spotlight beam, demonstrate each source alone, then both together. Each shadow attenuates only its own light's contribution. |
| Particle effects | Energy motes rise, drift, and fade around the cyan core, mostly against the dark recess. | Toggle particles independently of bloom. Their animation uses elapsed time so emission and lifetimes do not depend on rendering framerate. |
| Post-processing: bloom | Bright core, lamp face, and selected particles have a soft halo without washing out the white housing. | Toggle bloom at fixed exposure and camera position. The emissive surfaces stay bright with bloom off, while the surrounding halo disappears. |

All features run together in the same application. The UI also provides a global
pause/resume control and a restart control so comparisons can use identical
animation states. Feature comparisons should keep unrelated settings unchanged.

### Possible extensions

The initial target is the mandatory features and the four extras above. If time
allows after those are working and visually polished:

- **Ambient occlusion:** first candidate; subtle contact shading around supports,
  panel recesses, and drone joints, demonstrated with an independent toggle.
- **Minimap:** a simultaneous overhead inset showing the drone and route, rendered
  to a texture rather than only switching to the existing overhead camera.
- **Procedural water:** a shallow surrounding pool with shader-driven waves;
  would require revisiting the platform composition and water material.
- **Other ideas to revisit:** parameter-driven props, a genuinely frame-animated
  diagnostic texture, or coordinated changing lighting conditions.
