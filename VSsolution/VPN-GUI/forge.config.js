export const makers = [
    {
        name: '@electron-forge/maker-squirrel',
        config: {
            name: 'VPN-GUI',
            authors: 'Peter Debevec'
        },
        platforms: ['win32']
    },
    {
        name: '@electron-forge/maker-zip',
        platforms: ['win32', 'linux']
    },
];