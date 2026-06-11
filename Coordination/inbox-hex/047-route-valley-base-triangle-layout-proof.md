# Route Valley Base Triangle Layout Proof

Goal:
Restore the SPLITROOT Valley layout proof by routing the failing base-spacing issue
to the map lane and confirming the next policy run is fully green.

Steps:
- Inspect `Source/ArchonFactoryCanary/Private/Tests/ArchonSplitrootValleyLayoutTests.cpp`
  around line 177 and the current `splitroot_valley` base placements.
- Coordinate with Quarry if the fix belongs in terrain/layout data rather than
  the test threshold.
- Re-run the policy automation or the narrow valley layout proof after the map
  data changes.

Done-when:
- `ArchonFactory.Valley.ThreeFactionBasesFormTriangle` passes.
- `Saved/Proof/last-policy-automation.log` no longer reports
  `Expected 'Lenswright and Kinwild far apart' to be true`.
- The result notes whether this was a map-placement fix or a test-threshold fix.

Report-to:
Gauge/Rook coordination. This was observed during Gauge's 2026-06-11 control
defaults pass after the supervisor boundary build adopted commit `c06858d`.

Evidence:
- Failing test: `ArchonFactory.Valley.ThreeFactionBasesFormTriangle`
- Failure:
  `Expected 'Lenswright and Kinwild far apart' to be true. [C:\Users\Jonathan\Projects\Users\Splitroot\Source\ArchonFactoryCanary\Private\Tests\ArchonSplitrootValleyLayoutTests.cpp(177)]`
- Build adoption itself succeeded; this is an unrelated map-layout automation
  failure, not a blocker for the control default change.
