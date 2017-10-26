require 'mxx_ru/binary_unittest'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		'sample/timertt/wheel/trivial_preallocated/prj.ut.rb',
		'sample/timertt/wheel/trivial_preallocated/prj.rb' )
)

