# QUTMS_AV_Sim Jazzy Migration Checklist

## Phase 1: Core Migration (Completed ✅)
- [x] Update package.xml dependencies
- [x] Update CMakeLists.txt for GZ Sim
- [x] Migrate vehicle plugin header to System interface
- [x] Rewrite vehicle plugin source for ECS
- [x] Update utils.hpp to remove Gazebo Classic dependencies
- [x] Update URDF plugin tags for GZ Sim
- [x] Update LIDAR sensor configuration
- [x] Convert launch files to ros_gz_sim
- [x] Create migration documentation
- [x] Update README with migration notice

## Phase 2: Testing & Completion (In Progress ⏳)

### Critical Path
- [x] **Migrate cone detection plugin**
  - [x] Update `gazebo_cone_detection.hpp` header
  - [x] Rewrite `gazebo_cone_detection.cpp` source
  - [x] Update `utils.hpp` in cone detection plugin
  - [ ] Test cone detection functionality
  
- [x] **Rename world files**
  - [x] Run `scripts/rename_worlds.sh`
  - [ ] Update any hardcoded `.world` references
  - [ ] Verify all tracks load in GZ Sim

- [ ] **Build & Test on Ubuntu 24.04**
  - [ ] Install ROS 2 Jazzy
  - [ ] Install GZ Sim Harmonic
  - [ ] Install ros_gz packages
  - [ ] Build workspace successfully
  - [ ] No compilation errors
  - [ ] No linking errors

### Functional Testing
- [ ] **Vehicle Plugin**
  - [ ] Vehicle spawns correctly
  - [ ] Initial pose set from world/track
  - [ ] Ackermann command control works
  - [ ] Twist command control works
  - [ ] Steering rate limits applied
  - [ ] Velocity control functional
  - [ ] Reset service works
  
- [ ] **Odometry & State**
  - [ ] Ground truth odometry published
  - [ ] Noisy odometry published
  - [ ] Motion noise applied correctly
  - [ ] Update rates correct (2 Hz update, 50 Hz publish)
  - [ ] Timestamps synchronized with sim time
  
- [ ] **TF & Visualization**
  - [ ] TF tree published correctly
  - [ ] map → base_link transform correct
  - [ ] Joint states published
  - [ ] Steering joints visualized
  - [ ] RViz displays vehicle correctly
  - [ ] Foxglove bridge connects
  
- [ ] **Sensors**
  - [ ] LIDAR scan data published
  - [ ] LIDAR frame correct
  - [ ] LIDAR noise/parameters correct
  - [ ] Cone detection publishes (after plugin migration)
  - [ ] Cone markers visualized
  
- [ ] **All Tracks Load**
  - [ ] small_track
  - [ ] small_track_2
  - [ ] small_oval
  - [ ] acceleration
  - [ ] BM_long_straight
  - [ ] BM_text_bubble
  - [ ] B_shape_02_03_2023
  - [ ] Jellybean_02_03_2023
  - [ ] Hairpin_02_03_2023
  - [ ] QR_Nov_2022
  - [ ] FSDS_Training

## Phase 3: Features & Enhancements (Future)

### ROS 2 Control Integration
- [ ] Install gz_ros2_control
- [ ] Create hardware interface
- [ ] Define controllers (steering, velocity)
- [ ] Configure controller manager
- [ ] Test with controller commands
- [ ] Update documentation

### Modular Plugin System
- [ ] Separate vehicle dynamics from sensors
- [ ] Create plugin configuration system
- [ ] Allow runtime plugin loading
- [ ] Document plugin API
- [ ] Create plugin examples

### Additional Sensors
- [ ] Camera sensor integration
  - [ ] Configure GZ Sim camera
  - [ ] Bridge to ROS topics
  - [ ] Test image publishing
  
- [ ] IMU sensor integration
  - [ ] Configure GZ Sim IMU
  - [ ] Bridge to ROS topics
  - [ ] Test IMU data
  
- [ ] GPS/GNSS sensor integration
  - [ ] Configure GZ Sim GNSS
  - [ ] Bridge to ROS topics
  - [ ] Test GPS data
  
- [ ] INS fusion plugin
  - [ ] IMU + GPS fusion
  - [ ] Publish fused odometry
  - [ ] Test accuracy

### Advanced Vehicle Dynamics
- [ ] Ackermann steering model
  - [ ] Replace bicycle model
  - [ ] Test steering behavior
  
- [ ] Tire dynamics
  - [ ] Slip model
  - [ ] Friction model
  - [ ] Test on different surfaces
  
- [ ] Suspension model
  - [ ] Spring-damper system
  - [ ] Weight transfer
  - [ ] Test dynamics
  
- [ ] Road conditions
  - [ ] Wet/dry grip variation
  - [ ] Surface coefficient
  - [ ] Test behavior changes

### Documentation & Quality
- [ ] API documentation (Doxygen)
- [ ] User tutorials
- [ ] Developer guide
- [ ] CI/CD pipeline
- [ ] Unit tests
- [ ] Integration tests

## Phase 4: Validation & Release

### Performance Testing
- [ ] Measure update rates
- [ ] Check CPU usage
- [ ] Monitor memory usage
- [ ] Optimize if needed

### Integration Testing
- [ ] Test with QUTMS_Driverless stack
- [ ] Verify perception pipeline
- [ ] Test planning/control integration
- [ ] Full autonomous lap simulation

### Documentation Review
- [ ] README up to date
- [ ] All guides tested
- [ ] API docs complete
- [ ] Examples working
- [ ] Known issues documented

### Release Preparation
- [ ] Create Humble branch
- [ ] Tag Jazzy release
- [ ] Update changelog
- [ ] Create release notes
- [ ] Announce to team

## Notes

### Blockers
- List any blocking issues here
- Dependencies not available
- API changes needed

### Questions
- Clarifications needed
- Design decisions pending
- Team input required

### Resources
- [JAZZY_MIGRATION_STATUS.md](./JAZZY_MIGRATION_STATUS.md)
- [MIGRATION_GUIDE.md](./MIGRATION_GUIDE.md)
- [QUICKSTART_JAZZY.md](./QUICKSTART_JAZZY.md)
- [MIGRATION_SUMMARY.md](./MIGRATION_SUMMARY.md)

---

**Last Updated:** 2026-02-28  
**Status:** Phase 2 complete - All plugins migrated, ready for testing on Jazzy
