from setuptools import find_packages, setup

package_name = 'mower_serial'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='ryanh',
    maintainer_email='ryanh@todo.todo',
    description='Debug serial bridge for low-level Arduino mower commands.',
    license='Apache-2.0',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'arduino_bridge = mower_serial.arduino_bridge:main',
        ],
    },
)
