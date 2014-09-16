require 'mxx_ru/binary_unittest'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		'sample/timertt/list/trivial/prj.ut.rb',
		'sample/timertt/list/trivial/prj.rb' )
)

