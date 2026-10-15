# 3DCGA Project

## Scene plan

### Concept: Reactor Inspection

A bright, clean energy exhibit in an open-air pavilion. A white-and-gold reactor
stands at the center of a pale circular platform, while a small inspection drone
flies around it. The visual direction is a premium technology showroom: simple
shapes, satin ceramic, polished metal, a bright sky, and restrained cyan accents.
Surface detail comes from manufactured panels, grooves, and markings.

The scene runs automatically in one application. Camera presets and independent
UI controls let us inspect each graphics technique, pause the scene, and capture
matching before/after screenshots. The following is the planned scope, rather
than a list of features already implemented.

### Layout and animation

- **Exhibit platform:** approximately 12 m across, raised slightly above a broad,
  pale ground plane. Its paneled surface receives shadows from the reactor and
  drone.
- **Reactor:** approximately 3.5 m tall, with a cylindrical pedestal, a luminous
  cyan core, a dark inner recess, white housing panels, three gold rings, and
  three structural supports. One outward-facing panel prominently displays
  normal-mapped grooves, screw heads, and an embossed emblem.
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

Build the following objects from reusable primitives in code. No external model
files are needed for the planned scene. Meshes need normals, texture coordinates,
and tangent data where normal maps are used.

| Object | Required parts | Construction / animation |
| --- | --- | --- |
| Ground | One broad ground plane | Static plane with a uniform pale material. |
| Exhibit platform | Circular base, top surface, narrow metallic edge | Static cylinders/discs; textured top surface. |
| Reactor pedestal | Base cylinder and metallic collar | Static cylinders. |
| Reactor core | Cyan inner cylinder and dark backing/recess | Static geometry with an emissive core material. |
| Reactor housing | Three white outer panels and one visible detail panel | Simple curved panels or box-like sections; detail panel has readable UVs for grooves and markings. |
| Reactor structure | Three gold rings and three supports | Static annular meshes and box/cylinder supports. |
| Drone body | Main shell, trim, and four rotor mounts | Root object follows the path; all other drone parts inherit its transform. |
| Drone rotors | Four hubs and four two-blade assemblies | Each blade assembly spins relative to its mount; blade shape makes rotation visible. |
| Drone sensor | Turning mount, tilting head, and dark lens | Nested transforms: body → turning mount → tilting head; lens follows the head. |
| Showcase lamp | Mast, overhead arm, lamp housing, luminous face | Static boxes/cylinders; spotlight position and direction match the fixture. |
| Sky background | Camera-centered cube | Samples the environment cubemap. |
| Energy particles | Reusable camera-facing quad | Particle instances have position, velocity, age, size, and opacity. |
| Path visualization | Sampled line along the four Bézier segments | Generated from the actual flight path; UI-controlled visibility. |

Required reusable mesh shapes are a plane/quad, box, cylinder/disc, annular ring,
and curved panel section. The drone hierarchy and its animation must be assembled
in code, including if individual mesh construction is later changed.

#### Texture and environment files

These are planned paths for assets to create or obtain. Each texture set is
shared by the relevant object's parts; gold, aluminium, the sensor lens, and
emissive surfaces can use constant material parameters instead of extra images.

| Planned file | Purpose |
| --- | --- |
| `assets/textures/platform_basecolor.png` | Pale floor-panel layout and clean exhibit markings. |
| `assets/textures/platform_roughness.png` | Roughness differences between panel surfaces and inset strips. |
| `assets/textures/platform_normal.png` | Shallow seams and manufactured floor detail. |
| `assets/textures/reactor_basecolor.png` | White housing panels, an emblem, and diagnostic labels. |
| `assets/textures/reactor_roughness.png` | Satin housing surfaces and smoother selected details. |
| `assets/textures/reactor_normal.png` | Recessed grooves, screw heads, and embossed detail; especially clear on the detail panel. |
| `assets/textures/drone_basecolor.png` | White shell, colored identification markings, and small labels. |
| `assets/textures/drone_roughness.png` | Surface-finish variation across the shell and trim regions. |
| `assets/textures/drone_normal.png` | Panel seams and small manufactured details. |
| `assets/textures/particle_soft.png` | Soft particle sprite with an alpha falloff; tinted cyan during rendering. |
| `assets/environment/pavilion_daylight.hdr` | Bright panoramic environment, converted to a cubemap for the background and reflections. |
| `assets/README.md` | Asset inventory, authorship, source links, licenses, and modifications, including our own generated assets. |

Base-color textures use sRGB color interpretation; roughness and normal maps are
linear data. Normal maps use a consistent tangent-space convention. Ceramic,
gold, aluminium, and dark sensor materials have separate metallic/roughness
defaults so they remain distinct when texture sampling is disabled.

#### Other required scene data and rendering resources

- **Scene data in code:** object transforms, material assignments, light
  settings, four Bézier segments, and camera presets. Separate model, animation,
  and scene-configuration files are not required for this scope.
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
- **Runtime-generated data:** primitive meshes, the path samples, the cumulative
  distance lookup table for constant-speed motion, particles, environment maps,
  shadow maps, and post-processing buffers. These are generated resources rather
  than additional asset files to download or commit.

### Planned mandatory features

| Feature | Where it appears | How we show it / UI controls |
| --- | --- | --- |
| Multiple viewpoints | The platform, flight route, drone, and reactor details. | Switch between showcase, overhead, follow, and close-up camera presets while the same scene runs. |
| Advanced shading: PBR | White ceramic, polished gold rings, aluminium supports, and the dark sensor. Curved surfaces expose highlight shape and material differences. | Compare PBR against the simple reference shading mode at a fixed camera, light setup, and exposure. |
| Material textures | Platform panels, reactor labels, drone markings, and spatial roughness variation. | Toggle material textures against constant material values. Keep normal mapping separately controllable. |
| Normal mapping | Reactor detail panel, platform seams, and drone panel detail. | In the reactor close-up, toggle normal mapping. Apparent grooves and screw heads flatten while the mesh silhouette stays unchanged. |
| Environment mapping | Gold reactor rings and metallic drone trim reflect the sky and distant environment. | Move the camera around the curved metal and toggle its environment-reflection contribution. Keep the background and direct lighting visible. |
| Smooth paths using at least three cubic Bézier curves | The drone flies a closed route comprising four smoothly joined cubic segments. | Toggle path movement and the rendered curve independently. The overhead view exposes the route; the showcase view exposes height changes. |
| Hierarchical transformations | Drone body → rotor mounts → spinning blades, and body → turning sensor mount → tilting sensor head. | Pause route movement while component animation continues, then toggle component animation separately. Use the drone close-up to show the nested motion. |

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
