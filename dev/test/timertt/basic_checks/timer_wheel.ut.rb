require 'mxx_ru/binary_unittest'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		'test/timertt/basic_checks/timer_wheel.ut.rb',
		'test/timertt/basic_checks/timer_wheel.rb' )
)

