
local sk_definition_writer = require('sk_definition_writer')
local sk_math = require('sk_math')
local static_export = require('tools.level_scripts.static_export')
local collision_export = require('tools.level_scripts.collision_export')
local portal_surfaces = require('tools.level_scripts.portal_surfaces')
local trigger = require('tools.level_scripts.trigger')
local world = require('tools.level_scripts.world')
local entities = require('tools.level_scripts.entities')
local signals = require('tools.level_scripts.signals')
local animation = require('tools.level_scripts.animation')
local dynamic_collision = require('tools.level_scripts.dynamic_collision_export')

sk_definition_writer.add_definition("level", "struct LevelDefinition", "_geo", {
    collisionQuads = sk_definition_writer.reference_to(collision_export.collision_objects, 1),
    collisionQuadCount = #collision_export.collision_objects,
    staticContent = sk_definition_writer.reference_to(static_export.static_content_elements, 1),
    staticContentCount = #static_export.static_content_elements,
    signalToStaticRanges = sk_definition_writer.reference_to(static_export.signal_ranges, 1),
    signalToStaticIndices = sk_definition_writer.reference_to(static_export.signal_indices, 1),
    signalToStaticCount = #static_export.signal_ranges,
    staticBoundingBoxes = sk_definition_writer.reference_to(static_export.static_bounding_boxes, 1),
    roomStaticMapping = sk_definition_writer.reference_to(static_export.room_ranges, 1),
    portalSurfaces = sk_definition_writer.reference_to(portal_surfaces.portal_surfaces, 1),
    portalSurfaceCount = #portal_surfaces.portal_surfaces,
    portalSurfaceMappingRange = sk_definition_writer.reference_to(portal_surfaces.portal_mapping_range, 1),
    portalSurfaceDynamicMappingRange = sk_definition_writer.reference_to(portal_surfaces.dynamic_mapping_range, 1),
    portalSurfaceMappingIndices = sk_definition_writer.reference_to(portal_surfaces.portal_mapping_data, 1),
    triggers = sk_definition_writer.reference_to(trigger.triggers, 1),
    triggerCount = #trigger.triggers,
    cutscenes = sk_definition_writer.reference_to(trigger.cutscene_data, 1),
    cutsceneCount = #trigger.cutscene_data,
    locations = sk_definition_writer.reference_to(trigger.location_data, 1),
    locationCount = #trigger.location_data,
    startLocation = trigger.find_location_index("start"),
    playerAnimatorIndex = animation.get_armature_index_with_name("player") or -1,
    world = world.world,
    boxDroppers = sk_definition_writer.reference_to(entities.box_droppers, 1),
    boxDropperCount = #entities.box_droppers,
    buttons = sk_definition_writer.reference_to(entities.buttons, 1),
    buttonCount = #entities.buttons,
    decor = sk_definition_writer.reference_to(entities.decor, 1),
    decorCount = #entities.decor,
    doors = sk_definition_writer.reference_to(entities.doors, 1),
    doorCount = #entities.doors,
    elevators = sk_definition_writer.reference_to(entities.elevators, 1),
    elevatorCount = #entities.elevators,
    fizzlers = sk_definition_writer.reference_to(entities.fizzlers, 1),
    fizzlerCount = #entities.fizzlers,
    pedestals = sk_definition_writer.reference_to(entities.pedestals, 1),
    pedestalCount = #entities.pedestals,
    signage = sk_definition_writer.reference_to(entities.signage, 1),
    signageCount = #entities.signage,
    signalOperators = sk_definition_writer.reference_to(signals.operators, 1),
    signalOperatorCount = #signals.operators,
    animations = sk_definition_writer.reference_to(animation.animated_nodes, 1),
    animationInfoCount = #animation.animated_nodes,
    switches = sk_definition_writer.reference_to(entities.switches, 1),
    switchCount = #entities.switches,
    dynamicBoxes = sk_definition_writer.reference_to(dynamic_collision.dynamic_boxes, 1),
    dynamicBoxCount = #dynamic_collision.dynamic_boxes,
    ballLaunchers = sk_definition_writer.reference_to(entities.ball_launchers, 1),
    ballLauncherCount = #entities.ball_launchers,
    ballCatchers = sk_definition_writer.reference_to(entities.ball_catchers, 1),
    ballCatcherCount = #entities.ball_catchers,
    clocks = sk_definition_writer.reference_to(entities.clocks, 1),
    clockCount = #entities.clocks,
    securityCameras = sk_definition_writer.reference_to(entities.security_cameras, 1),
    securityCameraCount = #entities.security_cameras,
    laserEmitters = sk_definition_writer.reference_to(entities.laser_emitters, 1),
    laserEmitterCount = #entities.laser_emitters,
    laserCubes = sk_definition_writer.reference_to(entities.laser_cubes, 1),
    laserCubeCount = #entities.laser_cubes,
    laserCatchers = sk_definition_writer.reference_to(entities.laser_catchers, 1),
    laserCatcherCount = #entities.laser_catchers,
})