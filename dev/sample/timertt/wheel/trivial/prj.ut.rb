require 'mxx_ru/binary_unittest'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		'sample/timertt/wheel/trivial/prj.ut.rb',
		'sample/timertt/wheel/trivial/prj.rb' )
)

