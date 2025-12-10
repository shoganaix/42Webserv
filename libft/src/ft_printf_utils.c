/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf_utils.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: macastro <macastro@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/10/26 18:35:45 by macastro          #+#    #+#             */
/*   Updated: 2025/02/21 13:16:23 by macastro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/libft.h"

void	ft_putarr_ints(int *ints, int size)
{
	int	i;

	i = 0;
	while (i < size)
	{
		ft_printf("%i\n", ints[i]);
		i++;
	}
}

void	ft_putarr_str(char **words)
{
	int	i;

	i = 0;
	while (words[i])
	{
		ft_printf("%s", words[i]);
		i++;
	}
	ft_printf("\n");
}

void	ft_putarr_str_nl(char **words)
{
	int	i;

	i = 0;
	while (words[i])
	{
		ft_printf("%s\n", words[i]);
		i++;
	}
}
