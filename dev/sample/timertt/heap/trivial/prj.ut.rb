require 'mxx_ru/binary_unittest'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		'sample/timertt/heap/trivial/prj.ut.rb',
		'sample/timertt/heap/trivial/prj.rb' )
)

