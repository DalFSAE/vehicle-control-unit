<a name="readme-top"></a>


<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://github.com/github_username/repo_name">
    <img src="images/dms_logo.jpg" alt="Logo" width="500" height="200">
  </a>

<h3 align="center">Real Time Vehicle Control Unit</h3>

  <p align="center">
    DMS-RTOS-VCU
    <br />
    <a href="https://github.com/AshtonDudley/DMS-RTOS-VCU"><strong>Explore the project »</strong></a>
    <br />
  </p>
</div>

<!-- GETTING STARTED -->
## Getting Started

To get a local copy up and running follow these steps.

### Prerequisites

This is an example of how to list things you need to use the software and how to install them.
* VSCode with the following extensions:
  ```sh
  STM32 VS Code Extension (https://marketplace.visualstudio.com/items?itemName=stmicroelectronics.stm32-vscode-extension)
  Cortex-Debug (https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug)
  ```
* STM32Cube Tools
    ```sh
    STM32CubeMX (https://www.st.com/en/development-tools/stm32cubemx.html)
    STM32CubeCLT (https://www.st.com/en/development-tools/stm32cubeclt.html)
    ```
* Arm GNU Toolchain
    ```sh
    https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
    ```

### Installation
## Contributing 
1. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
2. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
3. Push to the Branch (`git push origin feature/AmazingFeature`)
4. Open a Pull Request

## Continuous Integration

GitHub Actions validates the firmware build on every push and pull request using the existing CMake `Debug` preset.

To run the same build locally:

```sh
cmake --preset Debug
cmake --build --preset Debug
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>
